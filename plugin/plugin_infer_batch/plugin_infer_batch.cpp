//
// Created by cambricon on 18-12-13.
//

#include "plugin_infer_batch.hpp"

InferencerBatch :: InferencerBatch(int server_num,
                                   ModelType type,
                                   string cali_file,
                                   string cali_config_file,
                                   string config_file,
                                   string arg1,
                                   string arg2,
                                   string modelname,
                                   string output_model_file,
                                   bool fake,
                                   int infer_id, 
                                   int device_id,
                                   vector<int> cluster_ids) {
    server_num_ = server_num;
    // modeltype_ = type;
    // cali_file_  = cali_file.c_str();
    // cali_config_file_ = cali_config_file.c_str();
    // config_file_ = config_file.c_str();
    // arg1_ = arg1.c_str();
    // arg2_ = arg2.c_str();

    // output_model_file_ = output_model_file.c_str();
    // modelname_ = modelname;
    
    infer_id_ = infer_id;
    // device_id_ = device_id;

    mlu_output_idx_ = 0;
    mlu_input_cap_ = 4;
    mlu_output_cap_ = 16;

    fake_mlu_input = fake;
    // cluster_ids_ = cluster_ids;
    // if ( !cluster_ids.empty() ) {
    //     for (size_t i = 0; i < cluster_ids.size(); i++) {
    //         bind_bitmap_ = GenBindBitmap(device_id_, cluster_ids[i], bind_bitmap_);
    //     }
    // }
    mm_infer_ = new MMInfer(type,
                            cali_file, 
                            cali_config_file, 
                            config_file, 
                            arg1, 
                            arg2, 
                            output_model_file, 
                            modelname, 
                            device_id, 
                            cluster_ids);
}

string InferencerBatch :: name(){
    return "InferencerBatch";
}

bool InferencerBatch :: init_in_main_thread() {

    cur_offline_ = new cnOfflineInfo();
    // cur_offline_->data_parallel = 1;

    vector<cnOfflineInfo *> v;
    v.push_back(cur_offline_);
    pCtxt->offline.push_back(v);

    // init_device();

    // init_magicmind_model();
    // parse_model_info();

    // parse_input_para();
    // parse_output_para();

    server_ = InferBatchServer::create_infer_batch_server(server_num_, 0, mm_infer_, cur_offline_);
    return true;
}

bool InferencerBatch :: init_in_sub_thread() {
    // init_device();
    mail_box_ = new BlockQueue<int>(8);

    // if (!cluster_ids_.empty()) {
    //     for (auto cluster_id : cluster_ids_) {
    //         BindCluster(device_id_, cluster_id);
    //     }
    // }
    // if (bind_bitmap_ > 0) {
    //     BindCluster(device_id_, bind_bitmap_);
    // }
    return true;
}

bool InferencerBatch ::callback(TData *&pdata_in, vector<TData *>&pdatas_out) {
    if (pdata_in == nullptr) {
        pdatas_out.push_back(pdata_in);
        return true;
    }
    vector<TData*> finished_datas;
    get_finished_data(finished_datas);
    infer_cur_data(pdata_in);

    for(auto d : finished_datas){
        pdatas_out.push_back(d);
    }
    return true;
}

// =========================== private function ===============================
// bool InferencerBatch :: init_device() {
//     unsigned dev_num;
//     cnrtGetDeviceCount(&dev_num);
//     if (dev_num == 0) {
//         std::cout << "no device found" << std::endl;
//         exit(-1);
//     }
//     CNPEAK_CHECK_CNRT(cnrtSetDevice(device_id_));
//     return true;
// }

// void InferencerBatch::init_magicmind_model() {
//     if (check_file_exist(output_model_file_)) {
//         load_magicmind_model();
//     } else {
//         init_magicmind_parser();
//         init_magicmind_calibration();
//         init_magicmind_build();
//     }

//     // 获取模型输入输出规模
//     parse_model_info();

//     // 初始化output buffer
//     malloc_mlu_output_cache();
// }

// void InferencerBatch :: load_magicmind_model() {
//     std::cout << "model file already exist, skip parser..." << std::endl;
//     model_ = CreateIModel();
//     CNPEAK_CHECK_MM(model_->DeserializeFromFile(output_model_file_.c_str()));

//     printf("Model IO Info: \n");
//     for (const auto &name : model_->GetInputNames())
//     {
//         printf("   this model has %d inputs: %s\n", model_->GetInputNum(), name.c_str());
//     }
//     for (const auto &name : model_->GetOutputNames())
//     {
//         printf("   this model has %d outputs: %s\n", model_->GetOutputNum(), name.c_str());
//     }
// }

// void InferencerBatch :: init_magicmind_parser(){
//     if (modeltype_ == mCaffe) {
//         init_magicmind_parser_caffe();
//     } else if (modeltype_ == mPytorch) {
//         init_magicmind_parser_pytorch();
//     }
// }

// void InferencerBatch :: init_magicmind_parser_caffe(){
//     network_ = CreateINetwork();
//     caffe_parser_ = CreateIParser<magicmind::ModelKind::kCaffe, std::string, std::string>();
//     std::cout << "arg1: " << arg1_ << std::endl;
//     std::cout << "arg2: " << arg2_ << std::endl;
//     auto r = caffe_parser_->Parse(network_, arg2_, arg1_);
//     assert(r == magicmind::Status::OK());
//     if(modelname_ == "yolov3"){
//         const std::vector<int> order{0, 1, 2};
//         AppendYolov3Detection(network_, order);
//     }
    
//     int input_count = network_->GetInputCount();
//     Dims input_dims = network_->GetInput(0)->GetDimension();
    
//     // int output_count = network_->GetOutputCount();
//     Dims output_dims_0 = network_->GetOutput(0)->GetDimension();
//     Dims output_dims_1 = network_->GetOutput(1)->GetDimension();

//     for(int i=0; i<input_count; i++){
//         auto tensor = network_->GetInput(i);
//         tensor->SetDimension(input_dims);
//     }
// }

// void InferencerBatch :: init_magicmind_parser_pytorch(){
//     network_ = CreateINetwork();
//     pytorch_parser_ = CreateIParser<magicmind::ModelKind::kPytorch, std::string>();
//     std::cout << "arg1: " << arg1_ << std::endl;
//     pytorch_parser_->SetModelParam("pytorch-input-dtypes", {DataType::FLOAT32});

//     CNPEAK_CHECK_MM(pytorch_parser_->Parse(network_, arg1_));
//     if(modelname_ == "yolov3"){
//         const std::vector<int> order{0, 1, 2};
//         AppendYolov3Detection(network_, order);
//     }
    
//     int input_count = network_->GetInputCount();

//     for (int i=0; i < input_count; ++i) {
//         auto tensor = network_->GetInput(i);
//         tensor->SetDimension(Dims({8, 3, 678, 1024}));
//     }
//     // int input_count = network_->GetInputCount();
//     // Dims input_dims = network_->GetInput(0)->GetDimension();
    
//     // int output_count = network_->GetOutputCount();
//     // Dims output_dims_0 = network_->GetOutput(0)->GetDimension();
//     // Dims output_dims_1 = network_->GetOutput(1)->GetDimension();

//     // for(int i=0; i<input_count; i++){
//     //     auto tensor = network_->GetInput(i);
//     //     tensor->SetDimension(input_dims);
//     // }
// }

// void InferencerBatch :: init_magicmind_calibration(){
//     std::vector<std::string> data_paths;
//     std::string line;
//     std::ifstream fs(cali_file_.c_str());
//     while (getline(fs, line)) {
//         data_paths.push_back(line);
//     }
//     fs.close();
//     auto tensor = network_->GetInput(0);
//     auto input_dim = tensor->GetDimension();
//     if (input_dim.GetElementCount() == -1) {
//         std::cout << "Can not get elmentcount of calibration set, maybe there is one -1 in its shape." << std::endl;
//         return;
//     }
//     // auto dim_p = input_dim.GetDims();
//     // std::cout << dim_p.size() << std::endl;
//     auto input_dtype = network_->GetInput(0)->GetDataType();    
//     CVImageCalibData* fixed_cali = new CVImageCalibData(input_dim,
//                                              input_dtype,
//                                              data_paths.size(), 
// 					                         data_paths,
//                                              cali_config_file_);
//     ICalibrator* cali = CreateICalibrator(fixed_cali);
//     cali->SetQuantizationAlgorithm(QuantizationAlgorithm::LINEAR_ALGORITHM);

//     config_ = CreateIBuilderConfig();
//     CNPEAK_CHECK_MM(config_->ParseFromFile(config_file_));

//     CNPEAK_CHECK_MM(cali->Calibrate(network_, config_));
// }

// void InferencerBatch ::init_magicmind_build(){
//     builder_ = CreateIBuilder();
//     // config_ = CreateIBuilderConfig();
//     // auto r = config_->ParseFromFile(config_file_);
//     // assert(r == magicmind::Status::OK());
//     // config_->SetMLUArch({"mtp_370"});
//     // config_->ParseFromString(std::string(R"({"precision_mode": ")") + "int8_mixed_float16" + "\"}");
//     model_ = builder_->BuildModel("quantized mode", network_, config_);
//     // string data = "testcase/output/offlinemodel/yolov3_data";
//     // string model = "testcase/output/offlinemodel/yolov3_model";
//     CNPEAK_CHECK_MM(model_->SerializeToFile(output_model_file_.c_str()));
// }

// void InferencerBatch :: parse_model_info(){
//     cur_offline_->inputNum = model_->GetInputNum();
//     cur_offline_->outputNum = model_->GetOutputNum();
//     for(int i = 0; i < cur_offline_->inputNum; i++){
//         vector<int64_t> d = model_->GetInputDimension(i).GetDims();
//         cur_offline_->input_n[i] = (unsigned int)d[0];
//         cur_offline_->input_c[i] = (unsigned int)d[1];
//         cur_offline_->input_h[i] = (unsigned int)d[2];
//         cur_offline_->input_w[i] = (unsigned int)d[3];
//         cur_offline_->in_count[i] = model_->GetInputDimension(i).GetElementCount();
//         cur_offline_->input_datatype[i] = magicmind::DataTypeSize(model_->GetInputDataType(i));

//         // auto a = model_->GetInputDataType(i);
//         // auto b = DataTypeSize(a);
//         // sleep(1);
        
//     }
//     for(int i = 0; i < cur_offline_->outputNum; i++){
//         vector<int64_t> d = model_->GetOutputDimension(i).GetDims();
//         int n = (unsigned int)d[0];
//         int c = d.size() > 1 ? (unsigned int)d[1] : 1;
//         int h = d.size() > 2 ? (unsigned int)d[2] : 1;
//         int w = d.size() > 3 ? (unsigned int)d[3] : 1;
//         cur_offline_->output_n[i] = n;
//         cur_offline_->output_c[i] = c;
//         cur_offline_->output_h[i] = h;
//         cur_offline_->output_w[i] = w;
//         cur_offline_->out_count[i] = model_->GetOutputDimension(i).GetElementCount();
//         cur_offline_->output_datatype[i] = magicmind::DataTypeSize(model_->GetOutputDataType(i));

//         // auto a = model_->GetOutputDimension(i);
//         // auto b = a.GetElementCount();
//         // sleep(1);
//     }
// }

void InferencerBatch ::infer_cur_data(TData*& pdata_in){
    data_cache_.push_back(pdata_in);

    auto cur_mlu_output = mlu_output_cache_[mlu_output_idx_];
    mlu_output_idx_ = ++mlu_output_idx_ % mlu_output_cap_;
    pdata_in->mlu_output = cur_mlu_output;

    InferBatchMsg *msg = new InferBatchMsg();
    msg->msg_in = pdata_in->mlu_input;
    msg->msg_out = pdata_in->mlu_output;
    msg->frame_id = pdata_in->id;
    msg->qout = mail_box_;
    server_->get_server_mailbox()->push(msg);
}

bool InferencerBatch::get_finished_data(vector<TData*>& pdatas){
    while(true){
        int frame_id;
        bool ret = mail_box_->pop_non_block(frame_id);
        if(!ret) break;
        TData* data = data_cache_[0];
        pdatas.push_back(data);
        data_cache_.erase(data_cache_.begin());
    }

    return true;
}

void InferencerBatch :: malloc_mlu_output_cache() {
    for (int i = 0; i < mlu_output_cap_; i++) {
        void** mlu_output_data = reinterpret_cast<void**>(malloc(sizeof(void*)*cur_offline_->outputNum));
        for(int j=0; j<cur_offline_->outputNum; j++){
            cnrtMalloc(&mlu_output_data[j], cur_offline_->out_count[j]);
        }
        mlu_output_cache_.push_back(mlu_output_data);
    }
}