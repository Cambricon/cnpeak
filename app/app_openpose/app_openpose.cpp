//
// Created by jiapeiyuan on 12/19/19.
//

#include "app_openpose.hpp"

DEFINE_string(pt, "models/openpose_caffe/pose_deploy_linevec.prototxt", "The prototxt file used to find net configuration");
DEFINE_string(caffemodel, "models/openpose_caffe/pose_iter_440000.caffemodel", "The caffemodel file used to find net configuration");
DEFINE_string(config_file, "app/app_openpose/build_config.json", "The JSON config file for parsing model");
DEFINE_string(cali_config, "app/app_openpose/cali_config.json", "The JSON config file for parsing model");
DEFINE_string(output_model, "models/openpose_caffe/openpose_model", "The output model generated by magicmind");
DEFINE_string(video_path, "data/videos/openpose.mp4", "The input video file path");
DEFINE_string(cali_filelist, "data/images/quant_openpose/file_list", "The calibration file list");
DEFINE_int32(threads, 1, "data_parallel*threads*model_parallel should <= 32");

int test_decode_cncv_openpose(int argc, char* argv[]){
    ::google::InitGoogleLogging(argv[0]);
    FLAGS_alsologtostderr = 1;
    namespace gflags = google;
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    vector<cnPipe *> pipes;
    int num = FLAGS_threads;
    int src_w = 1920;
    int src_h = 1080;

    for(int i=0; i<num; i++){
//        auto src = new RtspVideoProvider(i, FLAGS_rtsp_path, 0, true, true, src_w, src_h);
        // auto *src = new CncodecImageProvider(i, FLAGS_cali_filelist, true, true);
        auto src = new CncodecVideoProvider(i, FLAGS_video_path, 0, true, true, false, src_w, src_h, 25);
        auto idgen = new IdGenerator();
        auto rc = new CncvPreprocess({224,224}, true, true, true, {104,117,123}, {255,255,255}, 1);
        auto *infer = new Inferencer(mCaffe, 
                                        FLAGS_cali_filelist, 
                                        FLAGS_cali_config,
                                        FLAGS_config_file, 
                                        FLAGS_pt, 
                                        FLAGS_caffemodel, 
                                        "openpose", 
                                        FLAGS_output_model,
                                        false,
                                        0, 0);
        auto copyout = new InferMemCopyoutOperator(i);
        auto post = new OpenposePostProcess(0, true, {224, 224});
        auto saver = new CvImageSaver("outputs/yolov3");
        auto player = new HttpVideoPlayer(8088, 1000);

        cnPipe *pipe = new cnPipe(i, true, true);
        pipe->add(src);
        pipe->add(idgen);
        pipe->add(rc);
        pipe->add(infer);
        pipe->add(copyout);
        pipe->add(post);
        pipe->add(player);
        pipes.push_back(pipe);
    }

    for(int i=0; i<num; i++){
        pipes[i]->init();
    }

    Timer t;
    for(int i=0; i<num; i++){
        pipes[i]->start();
    }

    for(int i=0; i<num; i++){
        pipes[i]->sync();
    }
    t.duration("test_decode_cncv_yolov3 exec:");

    return 0;
}

/* ============================================================ */
int main(int argc, char* argv[])
{
    test_decode_cncv_openpose(argc, argv);
}

