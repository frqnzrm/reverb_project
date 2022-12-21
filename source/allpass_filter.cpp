/**
 * \file allpass_filter.cpp
 *
 * \brief Source for allpass_filter class
 *
 * \class allpass_filter
 * 
 */

#include "allpass_filter.h"

allpass_filter::allpass_filter(){
    bufidx_read = 0.f;
    bufidx_write = 0;
    buffer_step = 1.f;
    decreaser = -1.f;
    increaser = 0.f;
    write_done = false;
    read_done = false;
    step_diff = 0.f;
}

float allpass_filter::process(float input){
    float output;
    float bufout;
    
    if (bufsize > oldbufsize){
        step_diff = bufsize - oldbufsize;

        if ((int) (step_diff - decreaser * 0.5f) > 0.1f){
            decreaser += 1;
            buffer_step = 0.5f;
        }
        else{
            buffer_step = 1.f;
            if (write_done){
                decreaser = -1.f;
                bufidx_read = (float) bufidx_write;
                write_done = false;
                read_done = false;
                read_new_bufsize = false;
                step_diff = 0.f;
                oldbufsize = bufsize;
            }
        }
        if (bufidx_write >= bufsize - 1){
            write_done = true;
            read_new_bufsize = true;
        }
//        std::cout << "AP 1: " << "bufidx_read: " << bufidx_read << ", bufidx_write: " << bufidx_write << ", oldbufsize: " << oldbufsize << ", bufsize: " << bufsize << ", difference " << step_diff - decreaser * 0.5f << std::endl;
    }

    else if (bufsize < oldbufsize){
        if (step_diff == 0){
            if (bufidx_write > bufsize){
                bufsize_written = bufidx_write;
            }
            else bufsize_written = bufsize;
        }
        step_diff = oldbufsize - bufsize_written;
        if ((int) (step_diff - increaser) >= 0.1f){
            increaser += 1;
            buffer_step = 2.f;
        }
        else{
            buffer_step = 1.f;
            if (read_done){
                increaser = 0.f;
                bufidx_read = (float) bufidx_write;
                read_done = false;
                write_done = false;
                step_diff = 0.f;
                read_new_bufsize = false;
                oldbufsize = bufsize;
            }
        }
        if ((int) bufidx_read >= oldbufsize - 2) {
            read_done = true;
            read_new_bufsize = true;
        }
//        std::cout << "AP 2: " << "bufidx_read: " << bufidx_read << ", bufidx_write: " << bufidx_write << ", oldbufsize: " << oldbufsize << ", bufsize: " << bufsize << ", difference " << step_diff - increaser << ", read_done: " << read_done << std::endl;
    }
    
    // whats actually done
    bufout = input - feedback * read_buffer(bufidx_read, buffer_step);
    output = feedback * bufout + read_buffer(bufidx_read, buffer_step);
    
    buffer[bufidx_write] = bufout;
    
    bufidx_read = bufidx_read + buffer_step;
    bufidx_write = bufidx_write + 1;
    
    if (not read_new_bufsize) {if(bufidx_read >= (float) oldbufsize) bufidx_read = 0;}
    else {if(bufidx_read >= (float) bufsize) bufidx_read = 0;}
    if(bufidx_write>=bufsize) bufidx_write = 0;
    
    return output;
}

float allpass_filter::read_buffer(float idx, float buffer_step){
    float alpha = idx - (int) idx;
    int idx_2 = (int) idx + 1;
    if (idx_2 >= (float) oldbufsize && not read_new_bufsize) idx_2 = 0;
    if (read_new_bufsize && idx_2 >= (float) bufsize) idx_2 = 0;
    if (buffer_step <= 1.f) return (1-alpha) * buffer[(int) idx] + alpha * buffer[idx_2];
    else return 0.5f * (buffer[(int) idx] + buffer[idx_2]);
}


void allpass_filter::initBuffer(int bufsizeIn){
    buffer.clear();
    for (int i = 0; i <= bufsizeIn; i++) {
        buffer.push_back(0.0f);
    }
    oldbufsize = bufsizeIn;
}

void allpass_filter::setbuffer(int bufsizeIn){
    bufsize = bufsizeIn;
}

void allpass_filter::mute(){
    for (float & sample : buffer){
        sample = 0;
    }
}

void allpass_filter::setfeedback(float value){
    feedback = value;
}

float allpass_filter::getfeedback(){
    return  feedback;
}

bool allpass_filter::ready(){
    if (step_diff == 0.f){
//        std::cout << "true" << std::endl;
        return true;
    }
    else{
        return false;
    }
}
