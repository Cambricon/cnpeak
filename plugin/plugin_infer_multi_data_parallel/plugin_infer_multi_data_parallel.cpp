//
// Created by cambricon on 19-4-15.
//


#include "plugin_infer_multi_data_parallel.hpp"
#include "core/tools.hpp"

InferencerMultiDataParallel::InferencerMultiDataParallel(int server_num,
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
                                                         vector<int> cluster_ids)
{
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
    mlu_output_cap_ = 16;

    fake_mlu_input_ = fake;
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

InferencerMultiDataParallel::~InferencerMultiDataParallel() {
    for (int i = 0; i < mlu_output_cap_; i++) {
        for (int j = 0; j < MAX_BATCH; j++) {
            for (int o = 0; o < cur_offline_->outputNum; o++) {
                CNPEAK_CHECK_CNRT(cnrtFree(mlu_output_cache_[i][j][o]));
            }
            mlu_output_cache_[i][j] = nullptr;
        }
        mlu_output_cache_[i].clear();
    }
    mlu_output_cache_.clear();
}

string InferencerMultiDataParallel::name() {
    return "InferencerMultiDataParallel";
}

bool InferencerMultiDataParallel::init_in_main_thread() {
    cur_offline_ = new cnOfflineInfo();
    vector<cnOfflineInfo *> v {cur_offline_};
    pCtxt->offline.push_back(v);

    parse_model_info();
    // 初始化output buffer
    malloc_mlu_output_cache();

    // init_device();

    // init_magicmind_model();
    return true;
}

bool InferencerMultiDataParallel::init_in_sub_thread() {
    for (int i=0; i < server_num_; i++) {
        CreateWorker(i);
    }
    // if ( !cluster_ids_.empty() ) {
    //     for (auto cluster_id : cluster_ids_) {
    //         BindCluster(device_id_, cluster_id);
    //     }
    // }
    return true;
}

bool InferencerMultiDataParallel ::callback(TData *&pdata_in, vector<TData *>&pdatas_out) {
    if (pdata_in == nullptr) {
        pdatas_out.push_back(pdata_in);
        return true;
    }
    int output_buf_idx = mlu_output_idx_;
    mlu_output_idx_ = (mlu_output_idx_ + 1) % mlu_output_cap_;

    pdata_in->mlu_multi_outputs.clear();
    if (pdata_in->mlu_multi_inputs.empty()) {
        pdatas_out.push_back(pdata_in);
        return true;
    } else if (pdata_in->mlu_multi_inputs.size() > MAX_BATCH) {
        pdatas_out.push_back(pdata_in);
        return false;
    }

    output_idx_ = 0;
    input_idx_ = 0;

    int times = pdata_in->mlu_multi_inputs.size();

    int loop = times / server_num_;
    int res = times % server_num_;

    for (int i=0; i<loop; i++) {
        inference_multi(pdata_in, server_num_, output_buf_idx);
    }

    inference_multi(pdata_in, res, output_buf_idx);
    pdatas_out.push_back(pdata_in);
    return true;
}

// =========================== private function ===============================
// bool InferencerMultiDataParallel::init_device() {
//     unsigned dev_num;
//     cnrtGetDeviceCount(&dev_num);
//     if (dev_num == 0) {
//         std::cout << "no device found" << std::endl;
//         exit(-1);
//     }
//     CNPEAK_CHECK_CNRT(cnrtSetDevice(device_id_));

//     return true;
// }

// void InferencerMultiDataParallel::init_magicmind_model() {
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

// void InferencerMultiDataParallel::load_magicmind_model() {
//     std::cout << "model file already exist, skip parser..." << std::endl;
//     model_ = CreateIModel();
//     CNPEAK_CHECK_MM(model_->DeserializeFromFile(output_model_file_.c_str()));

//     printf("Model IO Info: \n");
//     for (const auto &name : model_->GetInputNames()) {
//         printf("   this model has %d inputs: %s\n", model_->GetInputNum(), name.c_str());
//     }
//     for (const auto &dim : model_->GetInputDimensions()) {
//         std::cout << "   model input dimensions are : " << dim << std::endl;
//     }
//     for (const auto &type : model_->GetInputDataTypes()) {
//         std::cout << "   model input data types are : " << type << std::endl;
//     }
//     for (const auto &name : model_->GetOutputNames()) {
//         printf("   this model has %d outputs: %s\n", model_->GetOutputNum(), name.c_str());
//     }
//     for (const auto &dim : model_->GetOutputDimensions()) {
//         std::cout << "   model output dimensions are : " << dim << std::endl;
//     }
//     for (const auto &type : model_->GetOutputDataTypes()) {
//         std::cout << "   model output data types are : " << type << std::endl;
//     }
// }

// void InferencerMultiDataParallel::init_magicmind_parser() {
//     if (modeltype_ == mCaffe) {
//         init_magicmind_parser_caffe();
//     } else if (modeltype_ == mPytorch) {
//         init_magicmind_parser_pytorch();
//     }
// }

// void InferencerMultiDataParallel::init_magicmind_parser_caffe() {
//     network_ = CreateINetwork();
//     caffe_parser_ = CreateIParser<magicmind::ModelKind::kCaffe, std::string, std::string>();
//     std::cout << "arg1: " << arg1_ << std::endl;
//     std::cout << "arg2: " << arg2_ << std::endl;
//     auto r = caffe_parser_->Parse(network_, arg2_, arg1_);
//     assert(r == magicmind::Status::OK());
//     if (modelname_ == "yolov3") {
//         const std::vector<int> order{0, 1, 2};
//         AppendYolov3Detection(network_, order);
//     }
    
//     int input_count = network_->GetInputCount();
//     Dims input_dims = network_->GetInput(0)->GetDimension();
    
//     // int output_count = network_->GetOutputCount();
//     Dims output_dims_0 = network_->GetOutput(0)->GetDimension();
//     Dims output_dims_1 = network_->GetOutput(1)->GetDimension();

//     for (int i=0; i<input_count; i++) {
//         auto tensor = network_->GetInput(i);
//         tensor->SetDimension(input_dims);
//     }
// }

// void InferencerMultiDataParallel::init_magicmind_parser_pytorch() {
//     network_ = CreateINetwork();
//     pytorch_parser_ = CreateIParser<magicmind::ModelKind::kPytorch, std::string>();
//     std::cout << "arg1: " << arg1_ << std::endl;
//     pytorch_parser_->SetModelParam("pytorch-input-dtypes", {DataType::FLOAT32});

//     CNPEAK_CHECK_MM(pytorch_parser_->Parse(network_, arg1_));
//     if (modelname_ == "yolov3") {
//         const std::vector<int> order{0, 1, 2};
//         AppendYolov3Detection(network_, order);
//     } else if (modelname_ == "retinaface") {
//         auto tensor = network_->GetInput(0);
//         tensor->SetDimension(Dims({1, 3, 678, 1024}));
//     } else if (modelname_ == "arcface") {
//         auto tensor = network_->GetInput(0);
//         tensor->SetDimension(Dims({1, 3, 112, 112}));
//     } else if (modelname_ == "dbnet") {
//         auto tensor = network_->GetInput(0);
//         tensor->SetDimension(Dims({1, 3, 736, 896}));
//     } else if (modelname_ == "crnn") {
//         auto tensor = network_->GetInput(0);
//         tensor->SetDimension(Dims({1, 1, 32, 100}));
//     }

// }

// void InferencerMultiDataParallel::init_magicmind_calibration() {
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

// void InferencerMultiDataParallel::init_magicmind_build() {
//     builder_ = CreateIBuilder();
//     model_ = builder_->BuildModel("quantized mode", network_, config_);
//     CNPEAK_CHECK_MM(model_->SerializeToFile(output_model_file_.c_str()));
// }

void InferencerMultiDataParallel::parse_model_info() {
    cur_offline_->inputNum = mm_infer_->get_inputs_num();
    cur_offline_->outputNum = mm_infer_->get_outputs_num();
    for (int i = 0; i < cur_offline_->inputNum; i++) {
        vector<int64_t> d = mm_infer_->get_input_dim(i);
        cur_offline_->input_n[i] = (unsigned int)d[0];
        cur_offline_->input_c[i] = (unsigned int)d[1];
        cur_offline_->input_h[i] = (unsigned int)d[2];
        cur_offline_->input_w[i] = (unsigned int)d[3];
        cur_offline_->in_count[i] = mm_infer_->get_input_count(i);
        cur_offline_->input_datatype[i] = mm_infer_->get_input_datatypesize(i);    
    }
    for (int i = 0; i < cur_offline_->outputNum; i++) {
        vector<int64_t> d = mm_infer_->get_output_dim(i);
        int n = (unsigned int)d[0];
        int c = d.size() > 1 ? (unsigned int)d[1] : 1;
        int h = d.size() > 2 ? (unsigned int)d[2] : 1;
        int w = d.size() > 3 ? (unsigned int)d[3] : 1;
        cur_offline_->output_n[i] = n;
        cur_offline_->output_c[i] = c;
        cur_offline_->output_h[i] = h;
        cur_offline_->output_w[i] = w;
        cur_offline_->out_count[i] = mm_infer_->get_output_count(i);
        cur_offline_->output_datatype[i] = mm_infer_->get_output_datatypesize(i);
    }
}

void InferencerMultiDataParallel::malloc_mlu_output_cache() {
    for (int i = 0; i < mlu_output_cap_; i++) {
        vector<void**> tmp_outputs;
        for (int j = 0; j < MAX_BATCH; j++) {
            void** mlu_output_data = reinterpret_cast<void**>(malloc(sizeof(void*) * cur_offline_->outputNum));
            for (int o = 0; o < cur_offline_->outputNum; o++) {
                cnrtMalloc(&mlu_output_data[o], cur_offline_->out_count[o] * cur_offline_->output_datatype[o]);
            }
            tmp_outputs.push_back(mlu_output_data);
        }
        mlu_output_cache_.push_back(tmp_outputs);
    }
}

void InferencerMultiDataParallel::CreateWorker(int i) {
    WorkInfo *task = new WorkInfo();
    auto in = new BlockQueue<std::pair<void*, void**>*>(1);
    auto out = new BlockQueue<int>(1);
    task->id = i;
    task->qin = in;
    task->qout = out;
    task->server.reset(new MultiDataInferWorker(mm_infer_, cur_offline_, in, out));
    workers_.push_back(task);
}


bool InferencerMultiDataParallel::inference_multi(TData*& pdata_in, int num, int buf_idx) {
    for (int i=0; i<num; i++) {

        WorkInfo* w = workers_[i];
        void* in = pdata_in->mlu_multi_inputs[input_idx_];
        void** out = mlu_output_cache_[buf_idx][input_idx_];
        pdata_in->mlu_multi_outputs.push_back(out);
        input_idx_ ++;
        output_idx_ ++;

        auto msg = new std::pair<void*, void**>();
        msg->first = in;
        msg->second = out;
        w->qin->push(msg);
    }

    for (int i=0; i<num; i++) {
        int result;
        WorkInfo* w = workers_[i];
        w->qout->pop(result);
    }
    return true;
}