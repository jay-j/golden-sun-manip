#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

// Needs to be run with sudo permissions. 
// There should be a way to add the program to the group `video` but this does not seem to work.

struct frame_buffer {
    int fd;
    struct fb_fix_screeninfo fixed_info;
    struct fb_var_screeninfo var_info;
    
    int32_t width;
    int32_t height;
    
    size_t buffer_size;
    char* buffer;
};

int main(){
    struct frame_buffer fb;
    
    // open the file for reading
    fb.fd = open("/dev/fb0", O_RDONLY);
    if (fb.fd == -1){
        printf("[ERROR] Cannot open framebuffer device.\n");
        exit(1);
    }
    printf("Framebuffer device opened successfully.\n");
    
    // Get variable screen information
    ioctl(fb.fd, FBIOGET_VSCREENINFO, &fb.var_info);
    
    // get fixed screen information
    ioctl(fb.fd, FBIOGET_FSCREENINFO, &fb.fixed_info);
    
    // example. I get 32 bits per pixel; 4 channel, 8 bits each
    printf("vinfo.bits_per_pixel: %d\n", fb.var_info.bits_per_pixel);
    
    fb.width = fb.var_info.xres;
    fb.height = fb.var_info.yres;
    
    fb.buffer_size = fb.width * fb.height * fb.var_info.bits_per_pixel / 8;
    
    printf("For a screen resolution of %d x %d the buffer size is %lu\n", fb.width, fb.height, fb.buffer_size);
    
    
    close(fb.fd);
    return 0;
}