#include "magicmind_infer.h"

MMInfer::MMInfer(ModelType type,
            string cali_file,
            string cali_config_file,
            string config_file,
            string arg1, 
            string arg2,
            string output_model_file,
            string modelname,
            int device_id,
            vector<int> cluster_ids) : modeltype_(type),
                                            cali_file_(cali_file.c_str()),
                                            cali_config_file_(cali_config_file.c_str()),
                                            config_file_(config_file.c_str()),
                                            arg1_(arg1.c_str()),
                                            arg2_(arg2.c_str()),
                                            output_model_file_(output_model_file.c_str()),
                                            modelname_(modelname.c_str())
{
    device_id_      = device_id;
    if (!cluster_ids.empty()) {
        for (size_t i = 0; i < cluster_ids.size(); i++) {
            bind_bitmap_ = GenBindBitmap(device_id, cluster_ids[i], bind_bitmap_);
        }
    }

    CNPEAK_CHECK_CNRT(cnrtSetDevice(device_id_));
    CNPEAK_CHECK_CNRT(cnrtQueueCreate(&queue_));

    init_magicmind_model();
    init_magicmind_engine();
    init_magicmind_tensor();

    if (bind_bitmap_ > 0) {
        std::cout << " call bindcluster" << std::endl;
        BindCluster(device_id_, bind_bitmap_);
    }
}

MMInfer::~MMInfer() {
    if (builder_ != nullptr) {
        builder_->Destroy();
        builder_ = nullptr;
    }
    if (network_ != nullptr) {
        network_->Destroy();
        network_ = nullptr;
    }
    if (config_ != nullptr) {
        config_->Destroy();
        config_ = nullptr;
    }
    if (model_ != nullptr) {
        model_->Destroy();
        model_ = nullptr;
    }
    if (engine_ != nullptr) {
        engine_->Destroy();
        engine_ = nullptr;
    }
    if (context_ != nullptr) {
        context_->Destroy();
        context_ = nullptr;
    }
    CNPEAK_CHECK_CNRT(cnrtQueueDestroy(queue_));

    for (auto input : inputs_) {
        input->Destroy();
    }
    inputs_.clear();
    for (auto output : outputs_) {
        output->Destroy();
    }
    outputs_.clear();
}

void MMInfer::init_magicmind_model() {
    

    if (check_file_exist(output_model_file_)) {
        load_magicmind_model();
    } else {
        init_magicmind_parser();
        init_magicmind_calibration();
        init_magicmind_build();
    }
}

void MMInfer::load_magicmind_model() {
    std::cout << "model file already exist, skip parser..." << std::endl;
    model_ = CreateIModel();
    CNPEAK_CHECK_MM(model_->DeserializeFromFile(output_model_file_.c_str()));

    printf("Model IO Info: \n");
    for (const auto &name : model_->GetInputNames()) {
        printf("   this model has %d inputs: %s\n", model_->GetInputNum(), name.c_str());
    }
    for (const auto &dim : model_->GetInputDimensions()) {
        std::cout << "   model input dimensions are : " << dim << std::endl;
    }
    for (const auto &type : model_->GetInputDataTypes()) {
        std::cout << "   model input data types are : " << type << std::endl;
    }
    for (const auto &name : model_->GetOutputNames()) {
        printf("   this model has %d outputs: %s\n", model_->GetOutputNum(), name.c_str());
    }
    for (const auto &dim : model_->GetOutputDimensions()) {
        std::cout << "   model output dimensions are : " << dim << std::endl;
    }
    for (const auto &type : model_->GetOutputDataTypes()) {
        std::cout << "   model output data types are : " << type << std::endl;
    }
    
}

void MMInfer::init_magicmind_parser(){
    if (modeltype_ == mCaffe) {
        init_magicmind_parser_caffe();
    } else if (modeltype_ == mPytorch) {
        init_magicmind_parser_pytorch();
    }
}

void MMInfer::init_magicmind_parser_caffe(){
    network_ = CreateINetwork();
    caffe_parser_ = CreateIParser<magicmind::ModelKind::kCaffe, std::string, std::string>();
    std::cout << "arg1: " << arg1_ << std::endl;
    std::cout << "arg2: " << arg2_ << std::endl;
    auto r = caffe_parser_->Parse(network_, arg2_, arg1_);
    assert(r == magicmind::Status::OK());
    if(modelname_ == "yolov3") {
        const std::vector<int> order{0, 1, 2};
        AppendYolov3Detection(network_, order);
    } else if (modelname_ == "yolov5") {
        AppendYolov5Detection(network_, 0.45, 0.65, 1000);
    }
    
    int input_count = network_->GetInputCount();

    for(int i=0; i<input_count; i++){
        Dims input_dims = network_->GetInput(i)->GetDimension();
        auto tensor = network_->GetInput(i);
        tensor->SetDimension(input_dims);
    }
}

void MMInfer::init_magicmind_parser_pytorch(){
    network_ = CreateINetwork();
    pytorch_parser_ = CreateIParser<magicmind::ModelKind::kPytorch, std::string>();
    std::cout << "arg1: " << arg1_ << std::endl;
    std::cout << "arg2: " << arg2_ << std::endl;
    pytorch_parser_->SetModelParam("pytorch-input-dtypes", {DataType::FLOAT32});

    CNPEAK_CHECK_MM(pytorch_parser_->Parse(network_, arg1_));
    if(modelname_ == "yolov3"){
        const std::vector<int> order{0, 1, 2};
        AppendYolov3Detection(network_, order);
    } else if (modelname_ == "yolov5") {
        AppendYolov5Detection(network_, 0.45, 0.65, 1000);
    }

    // parse input Dims
    std::vector<std::vector<int64_t>> input_dims;
    std::stringstream in(arg2_);
    std::string tmp;
    while (std::getline(in, tmp)) {
        std::stringstream in1(tmp);
        std::string tmp1;
        std::vector<int64_t> input_dim;
        while (std::getline(in1, tmp1, ',')) {
            auto x = stol(tmp1);
            input_dim.push_back(x);
        }
        input_dims.push_back(input_dim);
    }

    int input_count = network_->GetInputCount();
    CNPEAK_CHECK_PARAM(input_dims.size() == input_count); 
    for (int i = 0; i < input_count; ++i) {
        auto tensor = network_->GetInput(i);
        tensor->SetDimension(Dims(input_dims[i]));
    }
}

void MMInfer::init_magicmind_calibration(){
    std::vector<std::string> data_paths;
    std::string line;
    std::ifstream fs(cali_file_.c_str());
    while (getline(fs, line)) {
        data_paths.push_back(line);
    }
    fs.close();
    auto tensor = network_->GetInput(0);
    auto input_dim = tensor->GetDimension();
    if (input_dim.GetElementCount() == -1) {
        std::cout << "Can not get elmentcount of calibration set, maybe there is one -1 in its shape." << std::endl;
        return;
    }
    auto input_dtype = network_->GetInput(0)->GetDataType();

    CVImageCalibData* fixed_cali = new CVImageCalibData(input_dim,
                                             input_dtype,
                                             data_paths.size(), 
					                         data_paths,
                                             cali_config_file_);
    ICalibrator* cali = CreateICalibrator(fixed_cali);
    cali->SetQuantizationAlgorithm(QuantizationAlgorithm::LINEAR_ALGORITHM);

    config_ = CreateIBuilderConfig();
    CNPEAK_CHECK_MM(config_->ParseFromFile(config_file_));

    CNPEAK_CHECK_MM(cali->Calibrate(network_, config_));
}

void MMInfer::init_magicmind_build(){
    builder_ = CreateIBuilder();
    model_ = builder_->BuildModel("quantized mode", network_, config_);
    CNPEAK_CHECK_MM(model_->SerializeToFile(output_model_file_.c_str()));
}

void MMInfer::init_magicmind_engine(){
    engine_ = model_->CreateIEngine();
    context_ = engine_->CreateIContext();
}

void MMInfer::init_magicmind_tensor(){
    CNPEAK_CHECK_MM(CreateInputTensors(context_, &inputs_));
    
    auto infer_input_dims = model_->GetInputDimensions();
    for (uint32_t i = 0; i < inputs_.size(); ++i) {
        if (inputs_[i]->GetDataType() == magicmind::DataType::TENSORLIST) {
            printf("Input tensor list is not supported yet.\n");
            return;
        }
        inputs_[i]->SetDimensions(infer_input_dims[i]);
    }

    CNPEAK_CHECK_MM(CreateOutputTensors(context_, &outputs_));
    // CNPEAK_CHECK_MM(context_->InferOutputShape(inputs_, outputs_));
}

void MMInfer::set_input_data(void* ptr, int idx) {
    CNPEAK_CHECK_PARAM(idx < get_inputs_num());
    CNPEAK_CHECK_PARAM(ptr != nullptr);
    inputs_[idx]->SetData(ptr);
}

void MMInfer::set_output_data(void* ptr, int idx) {
    CNPEAK_CHECK_PARAM(idx < get_outputs_num());
    CNPEAK_CHECK_PARAM(ptr != nullptr);
    outputs_[idx]->SetData(ptr);
}

void MMInfer::infer() {
    CNPEAK_CHECK_MM(context_->Enqueue(inputs_, outputs_, queue_));
    CNPEAK_CHECK_CNRT(cnrtQueueSync(queue_));
}