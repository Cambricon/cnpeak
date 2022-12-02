//
// Created by cambricon on 19-10-23.
//
#include <cstring>
#include "infer_multidata_worker.hpp"

MultiDataInferWorker::MultiDataInferWorker(MMInfer* model,
                                           cnOfflineInfo* offlineInfo,
                                           BlockQueue<std::pair<void*, void**>*>* in,
                                           BlockQueue<int>* out){
    model_ = model;
    queue_in_ = in;
    queue_out_ = out;
    is_stop_ = false;
    cur_offline_ = offlineInfo;

    // device_id_ = device_id;
    // bind_bitmap_ = bind_bitmap;

    new thread(&MultiDataInferWorker::start_work, this);
}

void MultiDataInferWorker::start_work(){
    init_worker();
    while(true){
        if(is_stop_){
            return;
        }
        std::pair<void*, void**>* in_msg;
        queue_in_->pop(in_msg);
        void* in = in_msg->first;
        void** out = in_msg->second;
        forward(in, out);
        delete in_msg;
        queue_out_->push(0);
    }
}

void MultiDataInferWorker::init_worker(){
    // init_device();
    // parse_input_args();
    // parse_output_args();
    // if (bind_bitmap_ > 0) {
    //     BindCluster(device_id_, bind_bitmap_);
    // }
}

bool MultiDataInferWorker::forward(void* in, void** out){

    for (size_t i = 0; i < model_->get_inputs_num(); i++) {
        // inputs_[i]->SetData(in);
        model_->set_input_data(in, i);
    }

    for (size_t i = 0; i < model_->get_outputs_num(); i++) {
        // outputs_[i]->SetData(out[i]);
        model_->set_output_data(out[i], i);
    }

    // CNPEAK_CHECK_MM(context_->Enqueue(inputs_, outputs_, queue_));
    // cnrtQueueSync(queue_);
    model_->infer();

    return true;
}

// bool MultiDataInferWorker::init_device() {
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

// void MultiDataInferWorker::parse_input_args(){
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
// }


// void MultiDataInferWorker ::parse_output_args() {
// }