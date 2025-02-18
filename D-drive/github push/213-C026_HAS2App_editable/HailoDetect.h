
#ifndef _HAILO_DETECT_H
#define _HAILO_DETECT_H
//--------------------------------------------------
//! @file HailoDetect.h
//! @brief Hailo Object Detection モジュール
//--------------------------------------------------

#include <algorithm>
#include <future>
#include <stdio.h>
#include <stdlib.h>
#include <functional>
#include <string>
#include <opencv2/opencv.hpp>
#include "hailo/hailort.h"
#include "double_buffer.hpp"
#include "yolo_postprocess.hpp"
#include "hailo_objects.hpp"
#include "hailo_tensors.hpp"
#include "hailomat.hpp"
//#include "utils.hpp"
#include "Global.h"
#include "Sysdef.h"

using namespace std;

#define INPUT_COUNT (1)
#define OUTPUT_COUNT (9)
#define INPUT_FILES_COUNT (1)
#define HAILO_VSTREAM_TIMEOUT 1000

struct boundingbox
{
    float x1;
    float x2;
    float y1;
    float y2;
	int   flg;
    float score;
    int label;
};

class FeatureData
{
public:
    FeatureData(uint32_t buffers_size, float32_t qp_zp, float32_t qp_scale, uint32_t width, hailo_vstream_info_t vstream_info) : m_buffers(buffers_size), m_qp_zp(qp_zp), m_qp_scale(qp_scale), m_width(width), m_vstream_info(vstream_info)
    {
    }
    static bool sort_tensors_by_size(std::shared_ptr<FeatureData> i, std::shared_ptr<FeatureData> j) { return i->m_width < j->m_width; };

    DoubleBuffer m_buffers;
    float32_t m_qp_zp;
    float32_t m_qp_scale;
    uint32_t m_width;
    hailo_vstream_info_t m_vstream_info;
};


class HAILODetection
{
	private:
		hailo_device					Device;
		hailo_hef						Hef;
		hailo_configured_network_group	Network_group;
		hailo_activated_network_group 	Activated_network_group;
		size_t							Input_vstreams_size;
		size_t							Output_vstreams_size;
		hailo_input_vstream_params_by_name_t Input_vstream_params[INPUT_COUNT];
		hailo_output_vstream_params_by_name_t Output_vstream_params[OUTPUT_COUNT];
		size_t							Vstreams_infos_size;
		hailo_vstream_info_t			Vstreams_infos[INPUT_COUNT+OUTPUT_COUNT];
		vector<HailoDetectionPtr>		Detections;
		YoloParams*						Init_params;
		std::vector<std::shared_ptr<FeatureData>> Features;
		hailo_stream_raw_buffer_by_name_t Input_buffer;
		hailo_stream_raw_buffer_by_name_t Output_buffer[OUTPUT_COUNT];
	    uint8_t *Input_data;


		struct ModelParameter{
			int	modelType;
			int	inputImageW;
			int	inputImageH;
			int inputCount;
			int outputCount;
			string networkName;
			string configFile;

			ModelParameter(){}
			ModelParameter(int in_modelType, int in_inputImageW, int in_inputImageH, int in_inputCount, int in_outputCount, string in_networkName,string in_configFile)
			:modelType(in_modelType)
			,inputImageW(in_inputImageW)
			,inputImageH(in_inputImageH)
			,inputCount(in_inputCount)
			,outputCount(in_outputCount)
			,networkName(in_networkName)
			,configFile(in_configFile){}
		};
		static const ModelParameter MODEL_PARAMETER_TABLE[];
		ModelParameter modelParameter;
	public:
		inline vector<HailoDetectionPtr>& GetBboxesNms(){return Detections;}
		HAILODetection();
		~HAILODetection();
		bool hailoinit(eModelType modelType, void *modelBuffer, int size);
		hailo_status create_feature(hailo_output_vstream vstream, std::shared_ptr<FeatureData> &feature);
		hailo_status post_processing_all(int modeltype, YoloParams* param, vector<HailoDetectionPtr>& detections, std::vector<std::shared_ptr<FeatureData>> &features);
		hailo_status infer(HailoRGBMat& input_image);
		bool processImage(cv::Mat& oImg);
		std::tuple<float,float> GetHailoTemp(void);
		bool CheckDevice(void);
};

#endif
