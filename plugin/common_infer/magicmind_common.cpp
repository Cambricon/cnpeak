#include "magicmind_common.h"
#include "cn_api.h"

using namespace magicmind;

uint64_t GenBindBitmap(int dev_id, int thread_id, uint64_t bitmap) {
    int cluster_num = 0;
    CNPEAK_CHECK_CNRT(cnrtDeviceGetAttribute(&cluster_num, cnrtAttrClusterCount, dev_id));
    uint64_t bitmap_cur = 0x01;
    return bitmap | (bitmap_cur << (thread_id % cluster_num));
}

void BindCluster(int dev_id, uint64_t bind_bitmap) {
  std::cout << "bindcluster" << std::endl;
  // Set device id and get max cluster num
  CNPEAK_CHECK_CNRT(cnrtSetDevice(dev_id));
  int cluster_num = 0;
  CNPEAK_CHECK_CNRT(cnrtDeviceGetAttribute(&cluster_num, cnrtAttrClusterCount, dev_id));

  CNctxConfigParam param;
  param.unionLimit = CN_KERNEL_CLASS_UNION;

  // Set ctx
  CNcontext ctx;
  assert(cnCtxGetCurrent(&ctx) == CNresult::CN_SUCCESS);
  assert(cnSetCtxConfigParam(ctx, CN_CTX_CONFIG_UNION_LIMIT, &param) == CNresult::CN_SUCCESS);
  param.visibleCluster = bind_bitmap;
  assert(cnSetCtxConfigParam(ctx, CN_CTX_CONFIG_VISIBLE_CLUSTER, &param) == CNresult::CN_SUCCESS);
}

bool AppendYolov3Detection(INetwork* network, const std::vector<int>& order) {
    if ((int)order.size() != network->GetOutputCount()) {
        return false;
    }
    std::vector<int> perms{0, 2, 3, 1};  // NCHW -->NHWC  must be int type!!! cat not int64_t??
    const auto perms_size = perms.size();
    auto const_node = network->AddIConstNode(
        magicmind::DataType::INT32, magicmind::Dims({static_cast<int64_t>(perms_size)}), perms.data());
        
    std::vector<magicmind::ITensor*> output_tensors;
    for (auto i : order) {
        // add permute op, yolov3 detection layout is NHWC
        auto tensor = network->GetOutput(i);
        auto permute_node = network->AddIPermuteNode(tensor, const_node->GetOutput(0));
        output_tensors.push_back(permute_node->GetOutput(0));
    }
    const auto output_count = network->GetOutputCount();
    for (int i = 0; i < output_count; i++) {
        // unmark original output, now these tensors are internal tensor of network
        network->UnmarkOutput(network->GetOutput(0));
    }

    std::vector<float> bias_buffer{116, 90, 156, 198, 373, 326, 30, 61, 62,
                                    45, 59, 119, 10, 13, 16, 30, 33, 23};
    const auto bias_size = (long)bias_buffer.size();
    auto bias_node = network->AddIConstNode(
        magicmind::DataType::FLOAT32, magicmind::Dims({bias_size}), bias_buffer.data());
    auto detect_out = network->AddIDetectionOutputNode(
        output_tensors, bias_node->GetOutput(0));
    detect_out->SetLayout(magicmind::Layout::NONE, magicmind::Layout::NONE);
    detect_out->SetAlgo(magicmind::IDetectionOutputAlgo::YOLOV3);
    detect_out->SetConfidenceThresh(0.5);
    detect_out->SetNmsThresh(0.45);
    detect_out->SetScale(1.0);
    detect_out->SetNumCoord(4);
    detect_out->SetNumClass(80);
    detect_out->SetNumEntry(5);
    detect_out->SetNumAnchor(3);
    detect_out->SetNumBoxLimit(2048);
    detect_out->SetImageShape(416, 416);
    // auto detection_output_count = detect_out->GetOutputCount();
    for (int i = 0; i < detect_out->GetOutputCount(); ++i) {
        auto status = network->MarkOutput(detect_out->GetOutput(i));
    }
    return true;
}

bool AppendYolov5Detection(INetwork* network, const float conf, const float iou, const int64_t max_det) {
   
    // 使用permute算子将三个卷积层输出由NCHW(PyTorch模型数据均为NCHW摆放顺序)转为NHWC摆放顺序
    std::vector<int> perms{0, 2, 3, 1};  // NCHW -->NHWC  must be int type!!! cat not int64_t??
    const auto perms_size = perms.size();
    auto const_node = network->AddIConstNode(
        magicmind::DataType::INT32, magicmind::Dims({static_cast<int64_t>(perms_size)}), perms.data());
        
    std::vector<magicmind::ITensor*> output_tensors;
    for (int i = 0; i < network->GetOutputCount(); i++) {
        // add permute op, yolov3 detection layout is NHWC
        auto tensor = network->GetOutput(i);
        auto permute_node = network->AddIPermuteNode(tensor, const_node->GetOutput(0));
        output_tensors.push_back(permute_node->GetOutput(0));
    }
    const auto output_count = network->GetOutputCount();
    for (int i = 0; i < output_count; i++) {
        // unmark original output, now these tensors are internal tensor of network
        network->UnmarkOutput(network->GetOutput(0));
    }

    // anchors，按原始3个yolo层顺序填写
    std::vector<float> bias_buffer{10, 13, 16, 30, 33, 23, 30, 61, 62,
        45, 59, 119, 116, 90, 156, 198, 373, 326};
    const auto bias_size = (long)bias_buffer.size();
    auto bias_node = network->AddIConstNode(
        magicmind::DataType::FLOAT32, magicmind::Dims({bias_size}), bias_buffer.data());
    auto detect_out = network->AddIDetectionOutputNode(
        output_tensors, bias_node->GetOutput(0));
    detect_out->SetLayout(magicmind::Layout::NONE, magicmind::Layout::NONE);
    detect_out->SetAlgo(magicmind::IDetectionOutputAlgo::YOLOV5);
    detect_out->SetConfidenceThresh(conf);
    detect_out->SetNmsThresh(iou);
    detect_out->SetScale(1.0);
    detect_out->SetNumCoord(4);
    detect_out->SetNumClass(80);
    detect_out->SetNumEntry(5);
    detect_out->SetNumAnchor(3);
    detect_out->SetNumBoxLimit(max_det);
    detect_out->SetImageShape(416, 416);
    // auto detection_output_count = detect_out->GetOutputCount();
    for (int i = 0; i < detect_out->GetOutputCount(); ++i) {
        auto status = network->MarkOutput(detect_out->GetOutput(i));
    }
    return true;
}