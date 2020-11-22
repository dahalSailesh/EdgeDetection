// https://www.youtube.com/watch?v=jhmgti7OKlQ&t=0s

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
extern "C" {
#include "ppmFile.h"
}

#define TILE_WIDTH 32

__global__ void edgeDetectionTiled(int *d_width, int *d_height, unsigned char *d_input, unsigned char *d_output){

    // allocate shared memory
    __shared__ unsigned char sharedA[TILE_WIDTH];

    int bx = blockIdx.x;
    int by = blockIdx.y;
    int tx = threadIdx.x;
    int ty = threadIdx.y;

    int row = by * TILE_WIDTH + ty;
    int col = bx * TILE_WIDTH + tx;

    if(row >= (*d_height) || col >= (*d_width))
        return;

    // load shared memory
    int num_tiles = ((*d_width)+TILE_WIDTH - 1)/ TILE_WIDTH;
    for(int m = 0; m < num_tiles; m++){
        sharedA[tx] = d_input[0]; // what section do I want to copy?
    }
    // offset of the shared memory as well

    __syncthreads(); // barrier to wait for other threads before calculating

    int offset = 0, currT = 0, t2 = 0, t3 = 0, t4 = 0, t5 = 0;

    if(row > 0 && row < (*d_height) - 1 && col > 0 && col < (*d_width) - 1){
        offset = (row * (*d_width) + col) * 3;
        currT = (d_input[offset] + d_input[offset + 1] + d_input[offset + 2]);

        offset = (row * (*d_width) + (col+1)) * 3;
        t2 = (d_input[offset] + d_input[offset + 1] + d_input[offset + 2]);

        offset = (row * (*d_width) + (col-1)) * 3;
        t3 = (d_input[offset] + d_input[offset + 1] + d_input[offset + 2]);

        offset = ((row+1) * (*d_width) + col) * 3;
        t4 = (d_input[offset] + d_input[offset + 1] + d_input[offset + 2]);

        offset = ((row-1) * (*d_width) + col) * 3;
        t5 = d_input[offset] +  d_input[offset + 1] +  d_input[offset + 2];
    }

    __syncthreads(); // barrier to wait for other threads before replacing output

    offset = (row * (*d_width) + col) * 3; // curr offset
    if (abs(currT - t2) > 100 || abs(currT - t3) > 100 || abs(currT - t4) > 100 || abs(currT - t5) > 100){
        d_output[offset] = 255;
        d_output[offset + 1] = 255;
        d_output[offset + 2] = 255;
    }
    else{
        d_output[offset] = 0;
        d_output[offset + 1] = 0;
        d_output[offset + 2] = 0;
    }
}

int main (int argc, char *argv[]){
    double time_DTH, time_allocateHTD, time_kernel;
    clock_t begin_allocateHTD, end_allocateHTD, begin_kernel, end_kernel, begin_DTH, end_DTH;

    // Host variables (CPU)
    int width, height; //width & heigth for the image

    Image *inImage, *outImage; // ppmFile defined Image Struct
    unsigned char *data; // data of input image

    // Device variable (GPU)
    unsigned char *d_input, *d_output; // input image data
    int *d_width, *d_height; // width & height for the kernel

    if(argc != 3){
        printf("Incorrect number of input arguments. Include an input and output file, in that order.\n");
        return 0;
    }

    // Initializing values
    inImage = ImageRead(argv[1]);
    width = inImage->width;
    height = inImage->height;
    data = inImage->data;

    // Print the values of the images
    printf("Detecting edges on image: %s, with width: %d & height: %d\n", argv[1], width, height);

    // Grids based on size of the block
    dim3 blockD(TILE_WIDTH,TILE_WIDTH);
    dim3 gridD((width + blockD.x - 1)/blockD.x, (height + blockD.y - 1)/blockD.y);

    // Size of image pixels; 3 is the number of channels for RGB
    int image_size = width * height * 3;

    begin_allocateHTD = clock();

    // Allocate memory for GPU
    cudaMalloc((void**)&d_input, sizeof(unsigned char*) * image_size);
    cudaMalloc((void**)&d_output, sizeof(unsigned char*) * image_size);
    cudaMalloc((void**)&d_height, sizeof(int*));
    cudaMalloc((void**)&d_width, sizeof(int*));

    // Copy values from Host(CPU) to Device(GPU)
    cudaMemcpy(d_input, data, image_size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_width, &width, sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_height, &height, sizeof(int), cudaMemcpyHostToDevice);

    end_allocateHTD = clock();

    begin_kernel = clock();

    // Call edgeDetection() kernel on GPU.
    edgeDetectionTiled<<<gridD, blockD>>>(d_width, d_height, d_input, d_output);

    end_kernel = clock();

    // Create new image and clear the image to copy new image from the device.
    outImage = ImageCreate(width,height);
    ImageClear(outImage, 255, 255, 255);


    begin_DTH = clock();
    cudaDeviceSynchronize();

    // Copy new (edgeDetected) values from Device(GPU) to Host(CPU)
    cudaMemcpy(outImage->data, d_output, image_size, cudaMemcpyDeviceToHost);

    end_DTH = clock();

    // Write the new image onto the given file name
    ImageWrite(outImage, argv[2]);

    time_allocateHTD = (double)(end_allocateHTD-begin_allocateHTD) / CLOCKS_PER_SEC;
    printf("Allocation and Host to Device Time: %e s\n", time_allocateHTD);

    time_kernel = (double)(end_kernel-begin_kernel) / CLOCKS_PER_SEC;
    printf("Kernel Time: %e s\n", time_kernel);

    time_DTH = (double)(end_DTH-begin_DTH) / CLOCKS_PER_SEC;
    printf("Device to Host Time: %e s\n", time_DTH);

    printf("Total Time : %e s\n",time_allocateHTD + time_kernel + time_DTH);

    // Free memory
    free(inImage->data);
    free(outImage->data);
    cudaFree(d_input);
    cudaFree(d_output);
    cudaFree(d_width);
    cudaFree(d_height);

    return 0;
}
