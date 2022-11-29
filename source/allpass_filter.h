/**
 * \file allpass_filter.h
 *
 * \brief Header for allpass_filter class
 *
 * \class allpass_filter
 *
 * \brief Class defining a Schroeder allpass section.
 *
 * \author Fares Schulz
 *
 * \date 2022/10/14
 *
 */

#ifndef allpass_filter_h
#define allpass_filter_h

#include <iostream>
#include <vector>

class allpass_filter{
    
public:
    /// \brief allpass_filter::allpass_filter The constructor
    allpass_filter();
    
    /// \brief allpass_filter::setbuffer Sets the buffersize
    /// \param bufsizeIn the desired bufsize
    void setbuffer(int bufsizeIn);
    
    /// \brief allpass_filter::process The actual processing method
    /// \param input single sample input [float]
    /// \return the processed output [float]
    float process(float input);
    
    /// \brief allpass_filter::mute Mutes the buffer
    void mute();
    
    /// \brief allpass_filter::setfeedback Sets the feedback faktor
    /// \param val the desired feedback value
    void setfeedback(float val);
    
    /// \brief allpass_filter::getfeedback Gets the feedback value
    /// \return the current feedback value
    float getfeedback();
    
    /// \brief allpass_filter::initBuffer Initializes the buffer
    /// \details First the the buffer vector gets cleared and then the longest possible buffersize is written with zeros.
    /// \param bufsizeIn the initial buffersize
    void initBuffer(int bufsizeIn);
    
private:
    float feedback;
    std::vector<float> buffer;
    unsigned long bufsize;
    unsigned long oldbufsize = 0;
    unsigned long bufidx_read;
    unsigned long bufidx_write;
    bool interpolate = false;
    bool halfstep = false;
};

#endif /* allpass_filter_h */
