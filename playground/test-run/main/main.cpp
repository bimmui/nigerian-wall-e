#include "dl_model_base.hpp"
#include <cmath>

extern "C"
{
    extern const uint8_t _binary_sin_model_espdl_start[];
    extern const uint8_t _binary_sin_model_espdl_end[];
}

static inline size_t sin_model_size()
{
    return size_t(_binary_sin_model_espdl_end - _binary_sin_model_espdl_start);
}

static inline const char *sin_model_ptr()
{
    return reinterpret_cast<const char *>(_binary_sin_model_espdl_start);
}

static int degree = 90;

// shows how to use model->run() api.
void run1()
{
    const char *buf = sin_model_ptr(); // or (const char*)model_espdl
    size_t len = sin_model_size();     // or model_size()

    // Once the model is created, the input and output memeory is allocated.
    ESP_LOGI("MODEL", "size=%u", (unsigned)len);
    ESP_LOG_BUFFER_HEX_LEVEL("MODEL", buf, 16, ESP_LOG_INFO); // peek header

    dl::Model *model = new dl::Model(buf, fbs::MODEL_LOCATION_IN_FLASH_RODATA);

    std::map<std::string, dl::TensorBase *> model_inputs = model->get_inputs();
    std::map<std::string, dl::TensorBase *> model_outputs = model->get_outputs();
    if (inputs.empty() || outputs.empty())
    {
        ESP_LOGE("MODEL", "load() failed; empty I/O maps");
        delete model;
        return;
    }

    dl::TensorBase *model_input = model_inputs.begin()->second;
    dl::TensorBase *model_output = model_outputs.begin()->second;

    // quantize the float input and feed it into model.
    float input_v = degree * M_PI / 180;
    // Note that dl::quantize accepts inverse of scale as the second input, so we use DL_RESCALE here.
    int8_t qin = dl::quantize<int8_t>(input_v, DL_RESCALE(model_input->exponent));
    // assign quant_input_v to input.
    *reinterpret_cast<int8_t *>(model_input->data) = qin;
    // print input info and value.
    // model_input->print(true);

    model->run();

    // get the model output and dequantize it into float. Use pointer to do this element by element;
    // model_output->print(true);
    int8_t qout = *reinterpret_cast<int8_t *>(model_output->data);
    float out_v = dl::dequantize(qout, DL_SCALE(model_output->exponent));
    printf("sin(%d degree) = %f\n", degree, out_v);

    delete model;
}

// same as run1, different way to quantize input & dequantize output
void run2()
{
    const char *buf = sin_model_ptr(); // or (const char*)model_espdl
    size_t len = sin_model_size();     // or model_size()

    // Once the model is created, the input and output memeory is allocated.
    dl::Model *model = new dl::Model(buf, fbs::MODEL_LOCATION_IN_FLASH_RODATA);

    std::map<std::string, dl::TensorBase *> model_inputs = model->get_inputs();
    dl::TensorBase *model_input = model_inputs.begin()->second;
    std::map<std::string, dl::TensorBase *> model_outputs = model->get_outputs();
    dl::TensorBase *model_output = model_outputs.begin()->second;

    // quantize the float input and feed it into model.
    float input_v = degree * M_PI / 180;
    // create a TensorBase filled with 0 of shape [1, 1]
    dl::TensorBase *input_tensor = new dl::TensorBase({1, 1}, nullptr, 0, dl::DATA_TYPE_FLOAT);
    float *input_tensor_ptr = (float *)input_tensor->data;
    *input_tensor_ptr = input_v;
    model_input->assign(input_tensor);

    model->run();

    // get the model output and dequantize it into float.
    // create a TensorBase filled with 0 of shape [1, 1]
    dl::TensorBase *output_tensor = new dl::TensorBase({1, 1}, nullptr, 0, dl::DATA_TYPE_FLOAT);
    output_tensor->assign(model_output);
    float *output_tensor_ptr = (float *)output_tensor->data;
    float output_v = *output_tensor_ptr;

    printf("sin(%d degree) = %f\n", degree, output_v);

    delete input_tensor;
    delete output_tensor;
    delete model;
}

// shows how to use model->run(std::map<std::string, TensorBase *> &user_inputs, runtime_mode_t mode,
// std::map<std::string, TensorBase *> user_outputs) api
void run3()
{
    dl::Model *model = new dl::Model((const char *)model_espdl, fbs::MODEL_LOCATION_IN_FLASH_RODATA);
    // quantize the float input and feed it into model.
    float input_v = degree * M_PI / 180;
    // create a TensorBase filled with 0 of shape [1, 1]
    dl::TensorBase *input_tensor = new dl::TensorBase({1, 1}, nullptr, 0, dl::DATA_TYPE_FLOAT);
    float *input_tensor_ptr = (float *)input_tensor->data;
    *input_tensor_ptr = input_v;
    std::string input_name = model->get_inputs().begin()->first;
    std::map<std::string, dl::TensorBase *> input_map = {{input_name, input_tensor}};

    // create a TensorBase filled with 0 of shape [1, 1]
    dl::TensorBase *output_tensor = new dl::TensorBase({1, 1}, nullptr, 0, dl::DATA_TYPE_FLOAT);
    std::string output_name = model->get_outputs().begin()->first;
    std::map<std::string, dl::TensorBase *> output_map = {{output_name, output_tensor}};

    model->run(input_map, dl::RUNTIME_MODE_SINGLE_CORE, output_map);

    float *output_tensor_ptr = (float *)output_tensor->data;
    float output_v = *output_tensor_ptr;

    printf("sin(%d degree) = %f\n", degree, output_v);

    delete input_tensor;
    delete output_tensor;
    delete model;
}

extern "C" void app_main(void)
{
    run1();
    run2();
    run3();
}