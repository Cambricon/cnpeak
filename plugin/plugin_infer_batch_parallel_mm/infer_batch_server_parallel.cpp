//
// Created by cambricon on 19-11-28.
//

#include "infer_batch_server_parallel.hpp"

vector<InferBatchParallelServer*> InferBatchParallelServer::insts_ ;
InferBatchParallelServer* InferBatchParallelServer::create_infer_batch_server(int server_num,
                                                              int thread_id, 
                                                              MMInfer* model,
                                                              cnOfflineInfo* offineInfo){
    if(insts_.size() == 0){
        for(int i = 0; i < server_num; i++){
            auto inst = new InferBatchParallelServer(i, model, offineInfo);
            insts_.push_back(inst);
        }
    }
    int server_idx = thread_id % server_num;
    return insts_[server_idx];
}


BlockQueue<InferBatchMsg*>* InferBatchParallelServer::get_server_mailbox(){
    return mail_box_;
}


InferBatchParallelServer::InferBatchParallelServer(int server_id, MMInfer* model, cnOfflineInfo* offineInfo){
    // model_file_ = model_file;
    model_ = model;
    is_stop_ = false;
    server_id_ = server_id;
    msg_idx_ = 0;
    cur_offline_ = offineInfo;

    mlu_mem_idx_ = 0;
    mlu_mem_cap_ = 8;

    new thread(&InferBatchParallelServer::start_work, this);
}

void InferBatchParallelServer::start_work(){
    init_server();
    while(true){
        if(is_stop_){
            return;
        }
        InferBatchMsg* in_msg;
        mail_box_->pop(in_msg);

        for(int i = 0; i < model_->get_inputs_num(); i++) {
            // inputs_[i]->SetData(in_msg->msg_in[i]);
            // auto dim = inputs_[i]->GetDimensions();
            // inputs_[i]->SetDimensions(Dims({1, dim[1], dim[2], dim[3]}));
            model_->set_input_data(in_msg->msg_in[i], i);
        }

        mlu_output_cache_ = mlu_output_[mlu_mem_idx_];
        mlu_mem_idx_ = (mlu_mem_idx_ + 1) % mlu_mem_cap_;
        for (int i = 0; i < model_->get_outputs_num(); i++) {
            // outputs_[i]->SetData(mlu_output_cache_[i]);
            model_->set_output_data(mlu_output_cache_[i], i);
        }

        // context_->Enqueue(inputs_, outputs_, queue_);
        // cnrtQueueSync(queue_);
        model_->infer();

        for (int i = 0; i < model_->get_outputs_num(); i++) {
            in_msg->msg_out[i] = mlu_output_cache_[i];
        }
    }
}

void InferBatchParallelServer::init_server(){
    // init_device();
    // parse_input_args();
    // parse_output_args();
    // init_function();
    malloc_mlu_input_cache();
    malloc_mlu_output_cache();
    // init_ctxt();
    //init queue should be after geting batch_size_
    mail_box_ = new BlockQueue<InferBatchMsg*>(batch_size_);
}

// bool InferBatchParallelServer::init_device() {
//     unsigned dev_num;
//     cnrtGetDeviceCount(&dev_num);
//     if (dev_num == 0) {
//         std::cout << "no device found" << std::endl;
//         exit(-1);
//     }
//     CNPEAK_CHECK_CNRT(cnrtSetDevice(device_id_));
//     CNPEAK_CHECK_CNRT( cnrtQueueCreate(&queue_));

//     return true;
// }

// void InferBatchParallelServer::parse_input_args(){
//     engine_ = model_->CreateIEngine();
//     context_ = engine_->CreateIContext();

//     CNPEAK_CHECK_MM(CreateInputTensors(context_, &inputs_));
//     auto infer_input_dims = model_->GetInputDimensions();
//     for (uint32_t i = 0; i < inputs_.size(); ++i) {
//         if (inputs_[i]->GetDataType() == magicmind::DataType::TENSORLIST) {
//             printf("Input tensor list is not supported yet.\n");
//             return;
//         }
//         inputs_[i]->SetDimensions(infer_input_dims[i]);
//     }

//     CNPEAK_CHECK_MM(CreateOutputTensors(context_, &outputs_));
//     CNPEAK_CHECK_MM(context_->InferOutputShape(inputs_, outputs_));

//     //get n of model
//     batch_size_ = infer_input_dims[0][0];
//     inputNum = inputs_.size();
// }


// void InferBatchParallelServer ::parse_output_args() {
    
// }

bool InferBatchParallelServer::start_infer(){
    float event_time_use;

    for (int i = 0; i < model_->get_inputs_num(); i++) {
        // inputs_[i]->SetData(mlu_input_[i]);
        // auto dim = inputs_[i]->GetDimensions();
        //     inputs_[i]->SetDimensions(Dims({1, dim[1], dim[2], dim[3]}));
        model_->set_input_data(mlu_input_[i], i);
    }

    mlu_output_cache_ = mlu_output_[mlu_mem_idx_];
    mlu_mem_idx_ = (mlu_mem_idx_ + 1) % mlu_mem_cap_;
    for (int i = 0; i < model_->get_outputs_num(); i++) {
        // outputs_[i]->SetData(mlu_output_cache_[i]);
        model_->set_output_data(mlu_output_cache_[i], i);
    }

    // context_->Enqueue(inputs_, outputs_, queue_);
    // cnrtQueueSync(queue_);
    model_->infer();

    return true;
}

void InferBatchParallelServer :: malloc_mlu_input_cache(){
    mlu_input_ = reinterpret_cast<void**>(malloc(sizeof(void*) * cur_offline_->inputNum));
    for (int i = 0; i < model_->get_inputs_num(); i++) {
        int size = model_->get_input_size(i);
        CNPEAK_CHECK_CNRT(cnrtMalloc(&mlu_input_[i], size));
    }
}

void InferBatchParallelServer :: malloc_mlu_output_cache() {
    for (int i = 0; i < mlu_mem_cap_; i++) {
        int num = model_->get_outputs_num();
        void **mlu = (void**)(malloc(sizeof(void*)*num));
        for (int j = 0; j < num; j++) {
            int s = model_->get_output_size(j);
            CNPEAK_CHECK_CNRT(cnrtMalloc(&mlu[j], s));
        }
        mlu_output_.push_back(mlu);
    }
}
