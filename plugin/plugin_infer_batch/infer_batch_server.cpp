//
// Created by cambricon on 19-11-28.
//

#include "infer_batch_server.hpp"

vector<InferBatchServer*> InferBatchServer::insts_ ;
InferBatchServer* InferBatchServer::create_infer_batch_server(int server_num,
                                                              int thread_id, 
                                                              MMInfer* model,
                                                              cnOfflineInfo* offineInfo){
    if(insts_.size()==0){
        for(int i=0; i<server_num; i++){
            auto inst = new InferBatchServer(i, model, offineInfo);
            insts_.push_back(inst);
        }
    }
    int server_idx = thread_id % server_num;
    return insts_[server_idx];
}


BlockQueue<InferBatchMsg*>* InferBatchServer::get_server_mailbox(){
    return mail_box_;
}


InferBatchServer::InferBatchServer(int server_id, MMInfer* model, cnOfflineInfo* offineInfo){
    // model_file_ = model_file;
    model_ = model;
    is_stop_ = false;
    server_id_ = server_id;
    msg_idx_ = 0;
    cur_offline_ = offineInfo;

    mlu_mem_idx_ = 0;
    mlu_mem_cap_ = 8;

    batch_size_ = model_->get_input_dim(0)[0];

    new thread(&InferBatchServer::start_work, this);
}

// void InferBatchServer :: parse_model_info(){
//     cur_offline_->inputNum = inputs_.size();
//     cur_offline_->outputNum = outputs_.size();
//     for(int i=0; i<inputs_.size(); i++){
//         vector<int64_t> d = inputs_[i]->GetDimensions().GetDims();
//         cur_offline_->input_n[i] = (unsigned int)d[0];
//         cur_offline_->input_c[i] = (unsigned int)d[1];
//         cur_offline_->input_h[i] = (unsigned int)d[2];
//         cur_offline_->input_w[i] = (unsigned int)d[3];
//         cur_offline_->in_count[i] = inputs_[i]->GetDimensions().GetElementCount();
//     }
//     for(int i=0; i<outputs_.size(); i++){
//         vector<int64_t> d = outputs_[i]->GetDimensions().GetDims();
//         int n = (unsigned int)d[0];
//         int c = d.size() > 1 ? (unsigned int)d[1] : 1;
//         int h = d.size() > 2 ? (unsigned int)d[2] : 1;
//         int w = d.size() > 3 ? (unsigned int)d[3] : 1;
//         cur_offline_->output_n[i] = n;
//         cur_offline_->output_c[i] = c;
//         cur_offline_->output_h[i] = h;
//         cur_offline_->output_w[i] = w;
//         cur_offline_->out_count[i] = n*c*h*w;
//     }
//     cur_offline_->input_datatype = sizeof(float);
//     cur_offline_->output_datatype = sizeof(float);
// }


void InferBatchServer::start_work(){
    init_server();
    // parse_model_info();
    
    while(true){
        if(is_stop_){
            return;
        }
        InferBatchMsg* in_msg;
        mail_box_->pop(in_msg);

        bool isfull = batch_collect(in_msg);

        if(isfull){
#ifdef batch_d2d
            batch_d2d_copy();
#endif
            start_infer();
            send_result_back();
        }
    }
}

#ifdef batch_d2d
void InferBatchServer::batch_d2d_copy() {
    for (int b = 0; b < batch_size_; b++) {
        for (int i = 0; i < inputNum; i++) {
            int num = model_->get_input_size(i) / batch_size_;
            auto ptr = reinterpret_cast<void*>((char*)mlu_input_[i] + num * b);
            CNPEAK_CHECK_CNRT(cnrtMemcpy(ptr, msg_buffer_[b]->msg_in[i], num, CNRT_MEM_TRANS_DIR_DEV2DEV));
        }
    }
}
// void InferBatchServer::batch_d2d_copy(){
//     for(int i=0; i<inputNum; i++){
//         int num = inputSizeArray_[i]/batch_size_/2;
//         mlu_d2d_merge(msg_buffer_[0]->msg_in[i],
//                         msg_buffer_[1]->msg_in[i],
//                         msg_buffer_[2]->msg_in[i],
//                         msg_buffer_[3]->msg_in[i],
//                         num, mlu_input_cache_[i]);
//     }
// }
#endif

bool InferBatchServer::batch_collect(InferBatchMsg* msg){
    msg_buffer_.push_back(msg);

#ifndef batch_d2d
    for(int i=0; i<inputNum; i++){
        auto dst = (void *)((char *)mlu_input_cache_[i] + inputSizeArray_[i]/batch_size_ * msg_idx_);
        cnrtMemcpy(dst, msg->msg_in[i], inputSizeArray_[i]/batch_size_, CNRT_MEM_TRANS_DIR_DEV2DEV);
    }
    msg_idx_ ++;
#endif

    if((int)(msg_buffer_.size()) == batch_size_){
        return true;
    }
    else{
        return false;
    }
}

void InferBatchServer::send_result_back(){
#ifdef batch_d2d
    for (int b = 0; b < batch_size_; b++) {
        for (int i = 0; i < cur_offline_->outputNum; i++) {
            int num = model_->get_output_size(i) / batch_size_;
            auto ptr = reinterpret_cast<void*>((char*)mlu_output_cache_[i] + num * b);
            msg_buffer_[b]->msg_out[i] = ptr;
        }
    }

    // for(int i=0; i<outputNum; i++){
    //     int num = outputSizeArray_[i]/batch_size_/2;
    //     mlu_d2d_split(mlu_output_cache_[i], num,
    //                     msg_buffer_[0]->msg_out[i],
    //                     msg_buffer_[1]->msg_out[i],
    //                     msg_buffer_[2]->msg_out[i],
    //                     msg_buffer_[3]->msg_out[i]);

    // }
    for(int i=0; i<batch_size_; i++){
        auto msg_in = msg_buffer_[i];
        msg_in->qout->push(msg_in->frame_id);
        delete msg_in;
    }
#else
    for(int i=0; i<batch_size_; i++){
        auto msg_in = msg_buffer_[i];
        for(int j=0; j<outputNum; j++){
            auto src = (char *)mlu_output_cache_[j] + outputSizeArray_[j]/batch_size_ * i;
            cnrtMemcpy(msg_in->msg_out[j], src, outputSizeArray_[j]/batch_size_, CNRT_MEM_TRANS_DIR_DEV2DEV);
        }
        msg_in->qout->push(msg_in->frame_id);
        delete msg_in;
    }
#endif

    msg_buffer_.clear();
    msg_idx_ = 0;
}


void InferBatchServer::init_server(){
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

// bool InferBatchServer::init_device() {
//     unsigned dev_num;
//     cnrtGetDeviceCount(&dev_num);
//     if (dev_num == 0) {
//         std::cout << "no device found" << std::endl;
//         exit(-1);
//     }
//     CNPEAK_CHECK_CNRT(cnrtSetDevice(device_id_));
//     CNPEAK_CHECK_CNRT(cnrtQueueCreate(&queue_));

//     return true;
// }


// bool InferBatchServer ::init_ctxt() {
//     cnrtCreateRuntimeContext(&rt_ctx_, function2, nullptr);
//     cnrtSetRuntimeContextDeviceId(rt_ctx_, device_id_);
//     cnrtSetRuntimeContextChannel(rt_ctx_, (cnrtChannelType_t)(server_id_ % 4));
// //    cnrtSetRuntimeContextChannel(rt_ctx_, CNRT_CHANNEL_TYPE_DUPLICATE);
//     cnrtInitRuntimeContext(rt_ctx_, nullptr);
//     cnrtSetCurrentContextDevice(rt_ctx_);
// }

// bool InferBatchServer::init_function(){
//     cnrtCreateFunction(&function2);
//     cnrtCopyFunction(&function2, function);
//     cnrtQueueCreate(&queue_);
//     cnrtCreateNotifier(&noti_start);
//     cnrtCreateNotifier(&noti_end);
//     return true;
// }

// void InferBatchServer::parse_input_args(){
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

//     // cnrtLoadModel(&model, model_file_.c_str());
//     // int model_parallel;
//     // cnrtQueryModelParallelism(model, &model_parallel);

//     // string name = "subnet0";

//     // cnrtCreateFunction(&function);
//     // cnrtExtractFunction(&function, model, name.c_str());
//     // cnrtGetInputDataSize(&inputSizeArray_, &inputNum, function);
//     // cnrtGetOutputDataSize(&outputSizeArray_, &outputNum, function);

//     // for (int i = 0; i < inputNum; i++) {
//     //     cnrtGetInputDataShape(&input_dims[i], &input_dims_num[i], i, function);
//     // }

//     //get n of model
//     batch_size_ = infer_input_dims[0][0];
//     printf("infer batch size: %d\n", batch_size_);
//     inputNum = inputs_.size();
// }


// void InferBatchServer ::parse_output_args() {
//     // for (int i = 0; i < outputNum; i++) {
//     //     cnrtGetOutputDataShape(&output_dims[i], &output_dims_num[i], i, function);
//     // }
// }

bool InferBatchServer::start_infer(){
    // float event_time_use;

    for (size_t i = 0; i < model_->get_inputs_num(); i++) {
        model_->set_input_data(mlu_input_[i], i);
    }

    mlu_output_cache_ = mlu_output_[mlu_mem_idx_];
    mlu_mem_idx_ = (mlu_mem_idx_ + 1) % mlu_mem_cap_;
    for (size_t i = 0; i < model_->get_outputs_num(); i++) {
        model_->set_output_data(mlu_output_cache_[i], i);
    }


    // void* param[inputNum + outputNum];
    // for (int i = 0; i < inputNum; i++) {
    //     param[i] = mlu_input_cache_[i];
    // }
    // for (int i = 0; i < outputNum; i++) {
    //     param[inputNum + i] = mlu_output_cache_[i];
    // }

    // cnrtPlaceNotifier(noti_start, queue_);

    // context_->Enqueue(inputs_, outputs_, queue_);
    // cnrtQueueSync(queue_);
    model_->infer();

// #if 1// band cluster
//     cnrtInvokeParam_t invoke_param;
//     unsigned int affinity = 1 << server_id_;
//     invoke_param.invoke_param_type = CNRT_INVOKE_PARAM_TYPE_0;
//     invoke_param.cluster_affinity.affinity = &affinity;

//     CNPEAK_CHECK_CNRT(cnrtInvokeRuntimeContext(rt_ctx_, param, queue_, &invoke_param));
// #else
//     CNPEAK_CHECK_CNRT(cnrtInvokeRuntimeContext(rt_ctx_, param, queue_, nullptr));
// #endif

//     cnrtPlaceNotifier(noti_end, queue_);

//     if (cnrtQueueSync(queue_) == CNRT_RET_SUCCESS) {
//         cnrtNotifierDuration(noti_start, noti_end, &event_time_use);
//         inferencer_time += event_time_use;
// //        std::cout << " execution time: " << event_time_use << std::endl;
//     } else {
//         std::cout << " SyncStream error" << std::endl;
//     }

    return true;
}

void InferBatchServer :: malloc_mlu_input_cache(){
    // mlu_input_cache_ = reinterpret_cast<void**>(malloc(sizeof(void*)*inputNum));
    // for(int i=0; i<inputNum; i++){
    //     cnrtMalloc(&mlu_input_cache_[i], inputSizeArray_[i]);
    // }

    mlu_input_ = reinterpret_cast<void**>(malloc(sizeof(void*) * cur_offline_->inputNum));
    for (size_t i = 0; i < model_->get_inputs_num(); i++) {
        // int size = inputs_[i]->GetSize();
        int size = model_->get_input_size(i);
        CNPEAK_CHECK_CNRT(cnrtMalloc(&mlu_input_[i], size));
    }
}

void InferBatchServer :: malloc_mlu_output_cache() {
    // mlu_output_cache_ = reinterpret_cast<void**>(malloc(sizeof(void*)*outputNum));
    // for(int i=0; i<outputNum; i++){
    //     cnrtMalloc(&mlu_output_cache_[i], outputSizeArray_[i]);
    // }

    for (int i = 0; i < mlu_mem_cap_; i++) {
        int num = model_->get_outputs_num();
        void **mlu = (void**)(malloc(sizeof(void*)*num));
        for (int j = 0; j < num; j++) {
            int s = model_->get_output_size(i);
            CNPEAK_CHECK_CNRT(cnrtMalloc(&mlu[j], s));
        }
        mlu_output_.push_back(mlu);
    }
}
