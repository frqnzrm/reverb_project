/**
 * \file comb_filter.cpp
 *
 * \brief Source for comb_filter class
 * 
 * \class comb_filter
 * 
 */

#include "comb_filter.h"

comb_filter::comb_filter(){
    filtered_output = 0;
    bufidx_read = 0;
    bufidx_write = 0;
}

float comb_filter::process(float input){
    float output;
    
    if (bufsize > oldbufsize){
        if (bufidx_read >= oldbufsize && interpolate == false){
            bufidx_read = 0;
//            std::cout << "yy 1: " << "bufidx_read: " << bufidx_read << ", bufidx_write: " << bufidx_write << std::endl;
            output = buffer[bufidx_read];
            interpolate = true;
            halfstep = true;
            -- bufidx_read;
        }
        else if (bufidx_read != bufidx_write && interpolate == true){
//            std::cout << "yy 2: " << "bufidx_read: " << bufidx_read << ", bufidx_write: " << bufidx_write << ", bufsize: " << bufsize << std::endl;
            if (halfstep){
                output = 0.5 * (buffer[bufidx_read + 1] + buffer[bufidx_read]);
                halfstep = false;
            }
            else{
                output = (buffer[bufidx_read - 1] + buffer[bufidx_read] + buffer[bufidx_read + 1])/3;
                halfstep = true;
                -- bufidx_read;
            }
        }
        else if (bufidx_read == bufidx_write && interpolate == true){
//            std::cout << "yy 3: " << "bufidx_read: " << bufidx_read << ", bufidx_write: " << bufidx_write << std::endl;
            if (halfstep){
                output = 0.5 * (buffer[bufidx_read + 1] + buffer[bufidx_read]);
            }
            else{
                output = (buffer[bufidx_read - 1] + buffer[bufidx_read] + buffer[bufidx_read + 1])/3;
            }
            oldbufsize = bufsize;
            interpolate = false;
            halfstep = false;
        }
        else{
            output = buffer[bufidx_read];
        }
    }
    else if (bufsize < oldbufsize){
        if (bufidx_write >= bufsize){
            bufidx_write = 0;
        }
        if(bufidx_write < bufidx_read){
            if (bufidx_read == oldbufsize - 1){
                output = 0.5 * (buffer[bufidx_read] + buffer[0]);
            }
            else{
                output = 0.5 * (buffer[bufidx_read] + buffer[bufidx_read + 1]);
            }
            if(++bufidx_read >= oldbufsize){
                bufidx_read = 0;
            }
        }
        else if(bufidx_read < bufidx_write){
            output = 0.5 * (buffer[bufidx_read] + buffer[bufidx_read + 1]);
            ++ bufidx_read;
        }
        else{
            oldbufsize = bufsize;
            output = buffer[bufidx_read];
        }
    }
    else{
        output = buffer[bufidx_read];
    }
    
    buffer[bufidx_write] = input - (filtered_output*feedback);
    filtered_output = filtered_output + (1-damp)*(output-filtered_output);

    if(++bufidx_read>=bufsize) bufidx_read = 0;
    if(++bufidx_write>=bufsize) bufidx_write = 0;

    return output;
}

void comb_filter::initBuffer(int bufsizeIn){
    buffer.clear();
    for (int i = 0; i <= bufsizeIn; i++) {
        buffer.push_back(0.0f);
    }
}

void comb_filter::setbuffer(int bufsizeIn){
    if (oldbufsize == 0){
        oldbufsize = bufsizeIn;
    }
    else{
        oldbufsize = bufsize;
    }
    bufsize = bufsizeIn;
}

void comb_filter::mute(){
    for (float & sample : buffer){
        sample = 0;
    }
}

float comb_filter::read_buffer(float idx){
    float alpha = idx - (int) idx;
    int idx_2 = (int) idx + 1;
    if (idx_2 == oldbufsize - 1) idx_2 = 0;
    return (1-alpha) * buffer[(int) idx] + alpha * buffer[idx_2];
}

void comb_filter::setdamp(float val){
    damp = val;
}

float comb_filter::getdamp(){
    return damp;
}

void comb_filter::setfeedback(float val){
    feedback = val;
}

float comb_filter::getfeedback(){
    return feedback;
}
