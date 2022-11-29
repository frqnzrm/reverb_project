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
    bufidx_read= 0;
    bufidx_write = 0;
}

float allpass_filter::process(float input){
    float output;
    float bufout;
    
    if (bufsize > oldbufsize){
        if (bufidx_read >= oldbufsize && interpolate == false){
            bufidx_read = 0;
//            std::cout << "xx 1: " << "bufidx_read: " << bufidx_read << ", bufidx_write: " << bufidx_write << std::endl;
            bufout = input - feedback * buffer[bufidx_read];
            output = feedback * bufout + buffer[bufidx_read];
            interpolate = true;
            -- bufidx_read;
            halfstep = true;
        }
        else if (bufidx_read != bufidx_write && interpolate == true){
//            std::cout << "xx 2: " << "bufidx_read: " << bufidx_read << ", bufidx_write: " << bufidx_write << ", bufsize: " << bufsize << std::endl;
            if (halfstep){
                bufout = input - feedback * 0.5 * (buffer[bufidx_read + 1] + buffer[bufidx_read]);
                output = feedback * bufout + 0.5 * (buffer[bufidx_read + 1] + buffer[bufidx_read]);
                halfstep = false;
            }
            else{
                bufout = input - feedback * ((buffer[bufidx_read - 1] + buffer[bufidx_read] + buffer[bufidx_read +1])/3);
                output = feedback * bufout + ((buffer[bufidx_read - 1] + buffer[bufidx_read] + buffer[bufidx_read +1])/3);
                halfstep = true;
                -- bufidx_read;
            }
        }
        else if (bufidx_read == bufidx_write && interpolate == true){
//            std::cout << "xx 3: " << "bufidx_read: " << bufidx_read << ", bufidx_write: " << bufidx_write << std::endl;
            if (halfstep){
                bufout = input - feedback * 0.5 * (buffer[bufidx_read + 1] + buffer[bufidx_read]);
                output = feedback * bufout + 0.5 * (buffer[bufidx_read + 1] + buffer[bufidx_read]);
            }
            else{
                bufout = input - feedback * ((buffer[bufidx_read - 1] + buffer[bufidx_read] + buffer[bufidx_read +1])/3);
                output = feedback * bufout + ((buffer[bufidx_read - 1] + buffer[bufidx_read] + buffer[bufidx_read +1])/3);
            }
            oldbufsize = bufsize;
            interpolate = false;
            halfstep = false;
        }
        else{
            bufout = input - feedback * buffer[bufidx_read];
            output = feedback * bufout + buffer[bufidx_read];
        }
    }
    else if (bufsize < oldbufsize){
        if (bufidx_write >= bufsize){
            bufidx_write = 0;
        }
        if(bufidx_write < bufidx_read){
            if (bufidx_read == oldbufsize - 1){
                bufout = input - feedback * 0.5 * (buffer[bufidx_read] + buffer[0]);
                output = feedback * bufout + (0.5 * (buffer[bufidx_read] + buffer[0]));
            }
            else{
                bufout = input - feedback * 0.5 * (buffer[bufidx_read] + buffer[bufidx_read + 1]);
                output = feedback * bufout + (0.5 * (buffer[bufidx_read] + buffer[bufidx_read + 1]));
            }
            if(++bufidx_read >= oldbufsize){
                bufidx_read = 0;
            }
        }
        else if(bufidx_read < bufidx_write){
            bufout = input - feedback * 0.5 * (buffer[bufidx_read] + buffer[bufidx_read + 1]);
            output = feedback * bufout + (0.5 * (buffer[bufidx_read] + buffer[bufidx_read + 1]));
            ++ bufidx_read;
        }
        else{
            oldbufsize = bufsize;
            bufout = input - feedback * buffer[bufidx_read];
            output = feedback * bufout + buffer[bufidx_read];
        }
    }
    else{
        bufout = input - feedback * buffer[bufidx_read];
        output = feedback * bufout + buffer[bufidx_read];
    }

    buffer[bufidx_write] = bufout;
    
    if(++bufidx_read>=bufsize) bufidx_read= 0;
    if(++bufidx_write>=bufsize) bufidx_write = 0;
    
    return output;
}

void allpass_filter::initBuffer(int bufsizeIn){
    buffer.clear();
    for (int i = 0; i <= bufsizeIn; i++) {
        buffer.push_back(0.0f);
    }
}

void allpass_filter::setbuffer(int bufsizeIn){
    if (oldbufsize == 0){
        oldbufsize = bufsizeIn;
    }
    else{
        oldbufsize = bufsize;
    }
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

