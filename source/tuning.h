/**
 * \file tuning.h
 *
 * \brief Within this file the diffuse model initial state and different constants are defined.
 *
 * \author Fares Schulz
 *
 * \date 2022/10/14
 */

#include <cmath>

#ifndef tuning_h
#define tuning_h

/// number comb_filter instances within the diffuse model
const int   numcombs        = 8;
/// number allpass_filter instances within the diffuse model
const int   numallpasses    = 4;
/// scaling factor that defines how much the roomsize affects the comb_filter buffer sizes
const float scale_comb_buffer = 1.f;
/// scaling factor that defines how much the roomsize affects the allpass_filter buffer sizes
const float scale_allpass_buffer = 0.6f;
/// scaling factor that defines how much the roomsize affects the comb_filter feedback
const float scalefeedback    = 0.38f;
/// offset value that defines how much the roomsize affects the comb_filter feedback
const float offsetfeedback        = 0.6f;
/// initial roomsize when starting the client
const float initialroom        = 0.6f;
/// initial dampening of the comb_filter when starting the client
const float initialdamp        = 0.33f;
/// initial wet value of the comb_filter when starting the client
const float initialwet        = 1.f;
/// initial dry value of the comb_filter when starting the client
const float initialdry        = 0.f;
/// initial freeze mode state when starting the client
const bool  initialfreeze        = false;
/// gain value for the input signal
const float initialgain       = 1;
/// spreadvalue between the different output channels
const int   spreadvalue    = 23;
/// initial buffer sizes of the comb_filter instances
const int   comb_buffer_tuning[numcombs] = {1514, 1612, 1733, 1840, 1929, 2023, 2113, 2194};
/// initial buffer sizes of the allpass_filter instances
const int   allpass_buffer_tuning[numallpasses] = {556, 441, 341, 225};

#endif /* tuning_h */
