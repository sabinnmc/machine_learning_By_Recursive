/***********************************************************************
*
* Copyright (c) 2017-2019 Gyrfalcon Technology Inc. All rights reserved.
* See LICENSE file in the project root for full license information.
*
************************************************************************/

/**
 * \file GTILib.h
 * \brief GTI header file includes the public functions of GTI SDK library.
 */
#ifndef GYRFALCONTECH_GTILIB_H
#define GYRFALCONTECH_GTILIB_H

#ifdef GTISDK_DLL_EXPORT
#define GTISDK_API __declspec(dllexport)
#else
#define GTISDK_API
#endif

#define FILENAME_STRING_SIZE 1024

enum {
    GTI_OK = 0,
    GTI_INVALID_PARAM,
    GTI_NOT_IMPLEMENTED
};

enum GTI_DEVICE_STATUS {
    GTI_DEVICE_STATUS_ERROR,
    GTI_DEVICE_STATUS_ADDED,
    GTI_DEVICE_STATUS_REMOVED,
    GTI_DEVICE_STATUS_IDLE,
    GTI_DEVICE_STATUS_LOCKED,
    GTI_DEVICE_STATUS_RUNNING,
    GTI_DEVICE_STATUS_PENDING,
    GTI_DEVICE_STATUS_UNKNOWN,
    GTI_DEVICE_STATUS_BUT
};

enum GTI_DEVICE_TYPE {
    GTI_DEVICE_TYPE_ALL,
    GTI_DEVICE_USB_FTDI,
    GTI_DEVICE_USB_EUSB,
    GTI_DEVICE_PCIE,
    GTI_DEVICE_VIRTUAL,
    GTI_DEVICE_USB_NATIVE,
    GTI_DEVICE_DIRECT_EUSB,
    GTI_DEVICE_TYPE_UNKNOWN,
    GTI_DEVICE_TYPE_BUT
};

enum GTI_CHIP_MODE {
    FC_MODE,
    LEARN_MODE,
    SINGLE_MODE,
    SUBLAST_MODE,
    LASTMAJOR_MODE,
    LAST7x7OUT_MODE,
    SUM7x7OUT_MODE,
    GTI_CHIP_MODE_BUT
};

enum TENSOR_FORMAT {
    TENSOR_FORMAT_BINARY, // default is char
    TENSOR_FORMAT_BINARY_INTEGER,
    TENSOR_FORMAT_BINARY_FLOAT,
    TENSOR_FORMAT_TEXT,
    TENSOR_FORMAT_JSON,
    TENSOR_FORMAT_UNDEFINED,
    TENSOR_FORMAT_BUT,
};

enum GtiImageColorFormat {
    GTI_CF_BGR24_PLANAR,
    GTI_CF_RGB24_PLANAR,
    GTI_CF_UNDEFINED,
    GTI_CF_BUT,
};

typedef enum TENSOR_FORMAT GtiModelOutputFormat;


typedef void *GtiModel;
typedef void *GtiDevice;

typedef struct _GtiTensor {
    int width;
    int height;
    int depth;
    int stride;
    void *buffer;
    void *customerBuffer;
    int size; // buffer size;
    int format;
    void *tag;

    struct _GtiTensor *next; // GTI reserved for future use.
    void *callbackFn;        // GTI internal use, don't touch.
    void *privateData;
} GtiTensor;


typedef struct _ProcessTime {
    unsigned int write;
    unsigned int read;
} ProcessTime;

#ifdef __cplusplus
extern "C" {
#endif


/*!
\fn GtiModel *GtiCreateModel(const char *modelFile);
\brief Create a GTI Model object from a Model File
\param[in] modelFile -- A constant char string containing the name of the model file
\return -- A pointer to the created GtiModel object; null in case of errors
*/
GTISDK_API GtiModel *GtiCreateModel(const char *modelFile);

/*!
\fn GtiModel *GtiCreateModelFromBuffer(void *modelBuffer, int size);
\brief Create a GTI Model object given a buffer that contains the entire model
\param[in] modelBuffer -- A pointer to a buffer containing a model as a byte array
\param[in] modelSize -- An integer containing the model's size in bytes
\return -- A pointer to the created GtiModel object; null in case of errors
*/
GTISDK_API GtiModel *GtiCreateModelFromBuffer(void *modelBuffer, int size);

/*!
\fn int GtiDestroyModel(GtiModel *model);
\brief Destory a GTI Model object
\param[in] model -- A pointer to a GtiModel object
\return -- Always 1
*/
GTISDK_API int GtiDestroyModel(GtiModel *model);


/*!
\fn int GtiGetLayerInputTensorInfo(const char *modelBuffer, int layerIndex, GtiTensor *tensor);
\brief Parse the input model and return the input information of a layer in a linked list of tensor data structures.
\param[in] modelBuffer -- A pointer to the buffer that contains a model
\param[in] layerIndex -- The index of the layer of interest
\param[out] tensor    -- A head pointer to the returned list of tensor structures
\return -- The number of tensor structures in the returned list. Negative number if error
*/
GTISDK_API int GtiGetLayerInputTensorInfo(const char *modelBuffer, int layerIndex, GtiTensor *tensor);

/*!
\fn int GtiGetLayerOutputTensorInfo(const char *modelBuffer, int layerIndex, GtiTensor *tensor);
\brief Parse the input model and return the output information of a layer in a linked list of tensor data structures.
\param[in] modelBuffer -- A pointer to the buffer that contains a model
\param[in] layerIndex -- The index of the layer of interest
\param[out] tensor    -- A head pointer to the returned list of tensor structures
\return -- The number of tensor structures in the returned list. Negative number if error
*/
GTISDK_API int GtiGetLayerOutputTensorInfo(const char *modelBuffer, int layerIndex, GtiTensor *tensor);

/*!
\fn int GtiQueryModelLayerInfo(GtiModel *model, int layerIndex, GtiTensor **input, GtiTensor **output);
\brief Acquire information on a layer's input and output properties with a given model.
\param[in] model -- A pointer to a GtiModel object
\param[in] layerIndex -- which layer to query
\parma[out] input -- list of input tensors
\parma[out] output -- list of output tensors
\return -- Zero if the model information is available; negative is failed
*/
GTISDK_API int GtiQueryModelLayerInfo(GtiModel *model, int layerIndex, GtiTensor **input, GtiTensor **output);


/*!
\fn GtiTensor * GtiEvaluate(GtiModel *model,GtiTensor * input);
\brief Evaluate an input GtiTensor object
This function sends input data as a GtiTensor object to the established model.  The model
filters the data through all layers in the network, and output the results as a GtiTensor
object.
\param[in] model -- A pointer to the model object
\param[in] input -- A pointer to the input GtiTensor object
\return -- A pointer to the output GtiTensor object; NULL if failed
*/
GTISDK_API GtiTensor *GtiEvaluate(GtiModel *model, GtiTensor *input);

/*!
\fn GtiTensor *GtiEvaluateWithBuffer(GtiModel *model, GtiTensor *input, char *customerBuffer, int bufferSize);
\brief Evaluate an input GtiTensor object
This function sends input data as a GtiTensor object to the established model.  The model
filters the data through all layers in the network, and output the results as a GtiTensor
object.
\param[in] model -- A pointer to the model object
\param[in] input -- A pointer to the input GtiTensor object
\param[in] customerBuffer -- A pointer to an output buffer provided by the application
\param[in] bufferSize -- The size of provided output Buffer
\return -- A pointer to the output GtiTensor object; NULL if failed
*/
GTISDK_API GtiTensor *GtiEvaluateWithBuffer(GtiModel *model, GtiTensor *input, char *customerBuffer, int bufferSize);

/*!
\fn const char * GtiImageEvaluate(GtiModel *model, const char *image, int height, int width, int depth);
\brief Evaluate an input image object presented as an array of bytes
This function sends input data as an image arranged in an array of bytes to the established model.  The model
filters the data through all layers in the network, and output the results as an array of bytes.
\param[in] model -- a pointer to a model object
\param[in] image -- a char string, the input image file
\param[in] height -- height of image in number of pixels
\param[in] width -- width of image in number of pixels
\param[in] depth -- depth of image in number of channels per pixel
\return -- a pointer to the output data as an array of bytes
*/
GTISDK_API const char *GtiImageEvaluate(GtiModel *model, const char *image, int height, int width, int depth);



/*!
\fn const char * GtiGetSDKVersion();
\brief This function returns GTISDK version as a string
\return -- GTISDK version
*/
GTISDK_API const char *GtiGetSDKVersion();



/*!
\fn GtiDevice *GtiDeviceOpen(const char *deviceName);
\brief Create a GtiDevice object from the existing physical devices depending on the deviceName specified.
\param[in] deviceName -- Has to be one of the defined strings: "/dev/gti" or "/dev/bus/usb" or "/dev/sg" or "/dev/mmc"
\return -- A pointer to the available GtiDevice object, null pointer if failed
*/
GTISDK_API GtiDevice *GtiDeviceOpen(const char *deviceName);

/*!
\fn int GtiDeviceClose(GtiDevice *device);
\brief Destroy a GtiDevice object.
\param[in] device -- a pointer to the GtiDevice object
\return -- GTI_OK is successful, GTI_EINVAL if fail
*/
GTISDK_API int GtiDeviceClose(GtiDevice *device);

/*!
\fn int GtiDeviceIoctl(GtiDevice *device, unsigned long request, void *buffer);
\brief Carry out an IO operation on the selected device.  This is a wrapper for the device IOCTL function
\param[in] device -- a pointer to the GtiDevice object
\param[in] request -- request parameter for the device IOCTL
\param[in] buffer -- buffer parameter for the device IOCTL
\return -- device IOCTL functions return
*/
GTISDK_API int GtiDeviceIoctl(GtiDevice *device, unsigned long request, void *buffer);

/*!
\fn int GtiDeviceFlush(GtiDevice *device);
\brief Complete the devices write operation. This is a wrapper for the devices FLUSH function
\param[in] device -- a pointer to the GtiDevice object
\return -- device FLUSH functions return
*/
GTISDK_API int GtiDeviceFlush(GtiDevice *device);

/*!
\fn int GtiGetDeviceType(GtiDevice *device);
\brief Get the given device's type definition.
\param[in] device -- A pointer to the GtiDevice object
\return -- Enumerated device type
*/
GTISDK_API int GtiGetDeviceType(GtiDevice *device);

/*!
\fn int GtiGetBlockWriteOffset(GtiDevice *device);
\brief Get the offset of the current device write.
\param[in] device -- a pointer to the GtiDevice object
\return -- offset in byte for the current device write.
*/
GTISDK_API int GtiGetBlockWriteOffset(GtiDevice *device);


/*!
\fn int GtiDeviceRead(GtiDevice *device,unsigned char * buffer, unsigned int length);
\brief read data from the device.
\param[in] device -- A pointer to the GtiDevice
\param[in] buffer -- A pointer to the read buffer
\param[in] length -- read buffer length
\return -- the number of bytes read, minus numbers mean errors
*/
GTISDK_API int GtiDeviceRead(GtiDevice *device, unsigned char *buffer, unsigned int length);

/*!
\fn int GtiDeviceWrite(GtiDevice *device,unsigned char * buffer, unsigned int length);
\brief write data to the device.
\param[in] device -- A pointer to the GtiDevice
\param[in] buffer -- A pointer to the write buffer
\param[in] length -- write buffer length
\return -- the number of bytes wrote, minus numbers mean errors
*/
GTISDK_API int GtiDeviceWrite(GtiDevice *device, unsigned char *buffer, unsigned int length);


/*!
\fn int GtiGetGtiProcessTime(ProcessTime *pt);
\brief This function returns the last process time used by GTI hardware.
\param[out] pt -- A pointer to the ProcessTime struct which contains write and read time in microseconds
\return -- GTI_OK if successful
*/
GTISDK_API int GtiGetGtiProcessTime(ProcessTime *pt);

#ifdef __cplusplus
}
#endif

#endif /* ifndef GYRFALCONTECH_GTILIB_H */
