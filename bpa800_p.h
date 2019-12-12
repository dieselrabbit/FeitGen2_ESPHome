#include "esphome.h"

class BPA800_P : public Component, public LightOutput {
 public:
  BPA800_P(FloatOutput *red, FloatOutput *green, FloatOutput *blue, FloatOutput *brightness, FloatOutput *color_temp)
  {
    red_ = red;
    green_ = green;
    blue_ = blue;
    brightness_ = brightness;
    color_temp_ = color_temp;
    cold_white_temperature_ = 154.0;
    warm_white_temperature_ = 370.0;
  }

  LightTraits get_traits() override {
    // return the traits this light supports
    auto traits = LightTraits();
    traits.set_supports_brightness(true);
    traits.set_supports_rgb(true);
    traits.set_supports_rgb_white_value(false);
    traits.set_supports_color_temperature(true);
    traits.set_min_mireds(this->cold_white_temperature_);
    traits.set_max_mireds(this->warm_white_temperature_);
    return traits;
  }

  void setup_state(LightState *state) override {
    float remote_red, remote_green, remote_blue, current_brightness;
    state->remote_values.as_rgb(&remote_red, &remote_green, &remote_blue);
    state->current_values_as_brightness(&current_brightness);
    if (remote_red == remote_green && remote_red == remote_blue)
    {
      this->current_white_brightness_ = 1.0;
      this->current_rgb_brightness_ = 0.0;
    }
    else
    {
      this->current_white_brightness_ = 0.0;
      this->current_rgb_brightness_ = 1.0;
    }
  }

  void write_state(LightState *state) override {
    // This will be called by the light to get a new state to be written.
    // use any of the provided current_values methods
    float min_brightness = 0.06f;
    float remote_red, remote_green, remote_blue, remote_brightness, target_white_brightness, target_rgb_brightness;
    state->remote_values.as_rgb(&remote_red, &remote_green, &remote_blue);
    state->remote_values.as_brightness(&remote_brightness);

    float current_red, current_green, current_blue, current_brightness, current_color_temp, current_cw, current_ww;
    state->current_values_as_rgb(&current_red, &current_green, &current_blue);
    state->current_values_as_brightness(&current_brightness);
    current_color_temp = 1.0f - (state->current_values.get_color_temperature() - cold_white_temperature_) / (warm_white_temperature_ - cold_white_temperature_);

    target_rgb_brightness = 0.0;
    target_white_brightness = 0.0;
    if (!this->transitioning_) 
    {
      this->completion_ = 0.0;
      this->transitioning_ = true;
    }

    if (remote_red == remote_green && remote_red == remote_blue)
    {
      target_white_brightness = remote_brightness;
      target_rgb_brightness = 0.0;
    }
    else
    {
      target_white_brightness = 0.0;
      target_rgb_brightness = remote_brightness;
    }

    // Generally aligns to 1s transition time
    this->completion_ += 0.02;

    if (state->current_values == state->remote_values)
    {
      this->transitioning_ = false;
      this->completion_ = 1.0;
      current_white_brightness_ = target_white_brightness;
      current_rgb_brightness_ = target_rgb_brightness;
    }

    float lerp_red, lerp_green, lerp_blue, lerp_white;
    lerp_red = esphome::lerp(this->completion_, current_rgb_brightness_, target_rgb_brightness) * current_red;
    lerp_green = esphome::lerp(this->completion_, current_rgb_brightness_, target_rgb_brightness) * current_green;
    lerp_blue = esphome::lerp(this->completion_, current_rgb_brightness_, target_rgb_brightness) * current_blue;
    lerp_white = esphome::lerp(this->completion_, current_white_brightness_, target_white_brightness) * current_brightness;

    // Write red, green and blue to HW
    this->red_->set_level(lerp_red);
    this->green_->set_level(lerp_green);
    this->blue_->set_level(lerp_blue);
    this->color_temp_->set_level(current_color_temp);
    this->brightness_->set_level(lerp_white);
    //ESP_LOGD("custom", "completion: %f, lerp rgb: %f %f %f, lerp white: %f", completion_, lerp_red, lerp_green, lerp_blue, lerp_white);
  }

 protected:
  FloatOutput *red_;
  FloatOutput *green_;
  FloatOutput *blue_;
  FloatOutput *brightness_;
  FloatOutput *color_temp_;
  float cold_white_temperature_;
  float warm_white_temperature_;
  float current_rgb_brightness_;
  float current_white_brightness_;
  float completion_;
  bool transitioning_;
};