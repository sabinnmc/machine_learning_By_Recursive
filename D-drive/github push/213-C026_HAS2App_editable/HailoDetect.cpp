//--------------------------------------------------
//! @file HailoDetect.cpp
//! @brief Hailo Object Detection モジュール
//--------------------------------------------------

#include "HailoDetect.h"

using namespace std;

HAILODetection::HAILODetection()
{
	Device = NULL;
	Hef = NULL;
	Network_group = NULL;
	Activated_network_group = NULL;
	memset((void *)Input_vstream_params,0x00,sizeof(hailo_input_vstream_params_by_name_t)*INPUT_COUNT);
	memset((void *)Output_vstream_params,0x00,sizeof(hailo_output_vstream_params_by_name_t)*OUTPUT_COUNT);
	Init_params = NULL;
	Input_data = NULL;

}

const HAILODetection::ModelParameter HAILODetection::MODEL_PARAMETER_TABLE[] = {
	ModelParameter(),	// ﾀﾞﾐｰ
	ModelParameter{MODEL_TYPE_YOLOV5, 640, 640, 1, 3, "yolov5", "./config/yolov5.json"},
	ModelParameter{MODEL_TYPE_YOLOX_S, 640, 640, 1, 9,  "yolox_s", "./config/yolox_s.json"},
	ModelParameter{MODEL_TYPE_YOLOV4_TINY, 416, 416, 1, 2, "tiny_yolov4", "./config/yolov4_tiny.json"},
	ModelParameter{MODEL_TYPE_YOLOX_TINY, 416, 416, 1, 9,  "yolox_tiny", "./config/yolox_tiny.json"},
};

HAILODetection::~HAILODetection()
{
	if(Activated_network_group) (void)hailo_deactivate_network_group(Activated_network_group);
    if(Hef) (void)hailo_release_hef(Hef);
    if(Device) (void)hailo_release_device(Device);
	if( Input_data )	free(Input_data);
	for (auto &feature : Features)
	{
		feature->m_buffers.release_read_buffer();
	}
}

bool HAILODetection::hailoinit(eModelType modelType, void *modelBuffer, int size)
{
    hailo_status status = HAILO_UNINITIALIZED;
    hailo_configure_params_t config_params = {0};
    size_t network_group_size = 1;
	int output_pos = 0;
    size_t frame_size = 0;
	int i;

	modelParameter = MODEL_PARAMETER_TABLE[modelType];

	Input_vstreams_size = modelParameter.inputCount;
	Output_vstreams_size = modelParameter.outputCount;
	Vstreams_infos_size = modelParameter.inputCount + modelParameter.outputCount;

    status = hailo_create_pcie_device(NULL, &Device);
	if( status != HAILO_SUCCESS ){
		ERR("Hailo create device failed! %d",status);
		return(false);
	}
	status = hailo_create_hef_buffer(&Hef,modelBuffer,size);
	if( status != HAILO_SUCCESS ){
		ERR("Hailo create model failed! %d",status);
		return(false);
	}
    status = hailo_init_configure_params(Hef, HAILO_STREAM_INTERFACE_PCIE, &config_params);
	if( status != HAILO_SUCCESS ){
		ERR("Hailo initializing configure parameters failed! %d",status);
		return(false);
	}
    status = hailo_configure_device(Device, Hef, &config_params, &Network_group, &network_group_size);
	if( status != HAILO_SUCCESS ){
		ERR("Hailo configure devcie from hef failed! %d",status);
		return(false);
	}
    status = hailo_make_input_vstream_params(Network_group, true, HAILO_FORMAT_TYPE_AUTO, Input_vstream_params, &Input_vstreams_size);
	if( status != HAILO_SUCCESS ){
		ERR("Hailo making input virtual stream params failed! %d",status);
		return(false);
	}
    status = hailo_make_output_vstream_params(Network_group, true, HAILO_FORMAT_TYPE_AUTO, Output_vstream_params, &Output_vstreams_size);
	if( status != HAILO_SUCCESS ){
		ERR("Hailo making output virtual stream params failed! %d",status);
		return(false);
	}
	if((int)Input_vstreams_size != modelParameter.inputCount || (int)Output_vstreams_size != modelParameter.outputCount ){
		ERR("Hailo Expected one input vstream and three outputs vstreams");
		return(false);
	}
    status = hailo_hef_get_all_vstream_infos(Hef, NULL, Vstreams_infos, &Vstreams_infos_size);
	if( status != HAILO_SUCCESS ){
		ERR("Hailo getting virtual stream infos failed! %d",status);
		return(false);
	}
	if( (int)Vstreams_infos_size != modelParameter.inputCount + modelParameter.outputCount ){
		ERR("Hailo vstreams info size error!");
		return(false);
	}
	// タイムアウト設定
	for( i = 0; i < modelParameter.inputCount; i++ ){
		Input_vstream_params[i].params.timeout_ms = HAILO_VSTREAM_TIMEOUT;
	}
	for( i = 0; i < modelParameter.outputCount; i++ ){
		Output_vstream_params[i].params.timeout_ms = HAILO_VSTREAM_TIMEOUT;
	}
    status = hailo_activate_network_group(Network_group, NULL, &Activated_network_group);
	if( status != HAILO_SUCCESS ){
		ERR("Hailo activating network group failed! %d",status);
		return(false);
	}
	Init_params = init(modelParameter.configFile, modelParameter.networkName);
	if( Init_params == NULL ){
		ERR("Hailo model config initialized failed! %s",modelParameter.networkName.c_str());
		return(false);
	}

    Features.reserve(Output_vstreams_size);

    for (size_t i = 0; i < Vstreams_infos_size; i++) {
        if (HAILO_H2D_STREAM == Vstreams_infos[i].direction) {
            memcpy(Input_buffer.name, Vstreams_infos[i].name, HAILO_MAX_STREAM_NAME_SIZE);
            status = hailo_get_vstream_frame_size(&(Vstreams_infos[i]), &(Vstreams_infos[i].format), &frame_size);
			if (HAILO_SUCCESS != status)
			{
				ERR("Hailo getting input virtual stream frame size failed! %d",status);
				return false;
			}
            Input_buffer.raw_buffer.size = frame_size;
			Input_data = (uint8_t *)malloc(Input_buffer.raw_buffer.size);
			if( Input_data == NULL ){
				ERR("Hailo getting Input virtual stream frame size failed!");
				return false;
			}
            Input_buffer.raw_buffer.buffer = Input_data;
        } else {
			std::shared_ptr<FeatureData> feature(nullptr);
            memcpy(Output_buffer[output_pos].name, Vstreams_infos[i].name, HAILO_MAX_STREAM_NAME_SIZE);
            status = hailo_get_vstream_frame_size(&(Vstreams_infos[i]), &(Vstreams_infos[i].format), &frame_size);
			if (HAILO_SUCCESS != status)
			{
				ERR("Hailo getting output virtual stream frame size failed! %d",status);
				return false;
			}
            Output_buffer[output_pos].raw_buffer.size = frame_size;
			feature = std::make_shared<FeatureData>(static_cast<uint32_t>(frame_size), Vstreams_infos[i].quant_info.qp_zp,
                                            Vstreams_infos[i].quant_info.qp_scale, Vstreams_infos[i].shape.width, Vstreams_infos[i]);
            Output_buffer[output_pos].raw_buffer.buffer = reinterpret_cast<uint8_t *>(feature->m_buffers.get_write_buffer().data());
			Features.emplace_back(feature);
			output_pos++;
        }
    }


	return(true);

}


hailo_status HAILODetection::post_processing_all(int modeltype, YoloParams* param, std::vector<HailoDetectionPtr>& detections, std::vector<std::shared_ptr<FeatureData>> &features)
{
    auto status = HAILO_SUCCESS;


    std::sort(features.begin(), features.end(), &FeatureData::sort_tensors_by_size);
	// Gather the features into HailoTensors in a HailoROIPtr
	HailoROIPtr roi = std::make_shared<HailoROI>(HailoROI(HailoBBox(0.0f, 0.0f, 1.0f, 1.0f)));
	for (uint j = 0; j < features.size(); j++)
		roi->add_tensor(std::make_shared<HailoTensor>(reinterpret_cast<uint8_t *>(features[j]->m_buffers.get_write_buffer().data()), features[j]->m_vstream_info));

	// Perform the actual postprocess
	if( modeltype == MODEL_TYPE_YOLOV5 ){
		yolov5(roi, param);
	}
	else if( modeltype == MODEL_TYPE_YOLOV4_TINY ){
		tiny_yolov4(roi, param);
	}
	else if( modeltype == MODEL_TYPE_YOLOX_TINY ){
		tiny_yolox(roi, param);
	}
	else{
		yolox(roi, param);
	}

	detections = hailo_common::get_hailo_detections(roi);

    return status;
}


hailo_status HAILODetection::infer(HailoRGBMat& input_image)
{
    hailo_status status = HAILO_UNINITIALIZED;
	auto image_mat = input_image.get_mat();
	memcpy((void *)Input_data,image_mat.data,image_mat.total() * image_mat.elemSize());

    status = hailo_infer(Network_group,
        Input_vstream_params, &Input_buffer, 1,
        Output_vstream_params, Output_buffer, Output_vstreams_size,
        1);
	if (HAILO_SUCCESS != status)
	{
		std::cerr << "Inference failure with status = " << status << std::endl;
		return status;
	}
	status = post_processing_all(modelParameter.modelType, Init_params, std::ref(Detections), std::ref(Features));  // return box info

    return HAILO_SUCCESS;
}


bool HAILODetection::processImage(cv::Mat& oImg)
{
    auto status = HAILO_SUCCESS;
	cv::Mat oImgResized;
	// cv::Mat oRgb;
	// cv::resize(oImg, oImgResized, cv::Size(modelParameter.inputImageH, modelParameter.inputImageW));
	cv::resize(oImg, oImgResized, cv::Size(modelParameter.inputImageH, modelParameter.inputImageW),0,0,cv::INTER_NEAREST);	// cv::INTER_NEAREST処理早いが画質は落ちるので影響をみて
    // cv::cvtColor(oImgResized, oRgb, cv::COLOR_BGR2RGB);	// Inference内の受け渡しで実行する
    HailoRGBMat hImage = HailoRGBMat(oImgResized, "");
    status = infer(hImage);

	if( status != HAILO_SUCCESS ){
		ERR("Hailo inference failed! %d",status);
		return false;
	}
	return true;
}

bool HAILODetection::CheckDevice(void)
{
    auto status = HAILO_SUCCESS;
	hailo_device_identity_t identity;
	if(Device == NULL){
		ERR("Hailo not device open!");
		return false;
	}
	else{
		status = hailo_identify(Device, &identity);
		if( status != HAILO_SUCCESS ){
			ERR("Hailo Check Device failed! %d",status);
			return false;
		}
	}

	return true;
}

std::tuple<float,float> HAILODetection::GetHailoTemp(void)
{
	float ts0_temp = 0.0f;
	float ts1_temp = 0.0f;
    auto status = HAILO_SUCCESS;
	hailo_chip_temperature_info_t temp_info;
	if(Device == NULL){
		ERR("Hailo not device open!");
	}
	else{
		status = hailo_get_chip_temperature(Device, &temp_info);
		if( status != HAILO_SUCCESS ){
			ERR("Hailo get chip temperature failed! %d",status);
		}
		else{
			ts0_temp = (float)temp_info.ts0_temperature;
			ts1_temp = (float)temp_info.ts1_temperature;
		}
	}
	return {ts0_temp, ts1_temp};
}
