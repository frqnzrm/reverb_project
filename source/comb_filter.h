/**
 * \file comb_filter.h
 *
 * \brief Header for comb_filter class
 *
 * \class comb_filter
 *
 * \brief Class defining a lowpass feedback comb filter.
 *
 * \details This filter is a simple feedback comb filter, whose feedback is feeded into a simple lowpass filter.
 *
 * \author Fares Schulz
 *
 * \date 2022/10/14
 *
 */

#ifndef comb_filter_h
#define comb_filter_h

#include <iostream>
#include <vector>

class comb_filter{
    
public:
    /// \brief comb_filter::comb_filter The constructor
    comb_filter();
    
    /// \brief comb_filter::setbuffer Sets the buffersize
    /// \param bufsizeIn the desired bufsize
    void setbuffer(int bufsizeIn);
    
    /// \brief comb_filter::process The actual processing method
    /// \param input single sample input [float]
    /// \return the processed output [float]
    float process(float input);
    
    /// \brief comb_filter::mute Mutes the buffer
    void mute();
    
    /// \brief comb_filter::setdamp Sets the dampening factor
    /// \param val the desired dampening value
    void setdamp(float val);
    
    /// \brief comb_filter::getdamp Gets the dampening value
    /// \return the current dampening value
    float getdamp();
    
    /// \brief comb_filter::setfeedback Sets the feedback factor
    /// \param val the desired feedback value
    void setfeedback(float val);
    
    /// \brief comb_filter::getfeedback Gets the feedback value
    /// \return the current feedback value
    float getfeedback();
    
    /// \brief comb_filter::initBuffer Initializes the buffer
    /// \details First the the buffer vector gets cleared and then the longest possible buffersize is written with zeros.
    /// \param bufsizeIn the initial buffersize
    void initBuffer(int bufsizeIn);
    
private:
    float feedback;
    float damp;
    float filtered_output;
    std::vector<float> buffer;
    unsigned long bufsize;
    unsigned long oldbufsize = 0;
    unsigned long bufidx_read;
    unsigned long bufidx_write;
    bool interpolate = false;
    bool halfstep = false;
    float read_buffer(float value);
};


#endif /* comb_filter_h */
