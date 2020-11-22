#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "bmp.h"

// header
void edgeDetectionParallel(int height, int width, RGBTRIPLE (*image)[width]);
void *detectEdgeInParallel(void *argsArray);

#define THREADCOUNT 4
double time1;
clock_t begin, end;

typedef struct Arguments
{
    int width;
    int height;
    int startWidthIndex;
    int endWidthIndex;
    void *oldPic;
    void *newPic;
} Arguments;

void edgeDetectionParallel(int height, int width, RGBTRIPLE (*image)[width])
{
    // RGBTRIPLE currPix1 = image[11][8];
    // printf("%d %d %d\n", currPix1.rgbtRed, currPix1.rgbtBlue, currPix1.rgbtGreen);
    RGBTRIPLE newImage[height][width];
    Arguments *argsStructList[THREADCOUNT];
    pthread_t threads[THREADCOUNT];
    for (int i = 0; i < THREADCOUNT; i++)
    {
        argsStructList[i] = (Arguments *)malloc(sizeof(Arguments));
        argsStructList[i]->height = height;
        argsStructList[i]->width = width;

        // start and end index with no overlap
        int c = (width + THREADCOUNT - 1) / THREADCOUNT;
        int col_start = i * c;
        int col_end = col_start + c;
        if (col_end > width){
            col_end = width;
        }

        argsStructList[i]->startWidthIndex = col_start;
        argsStructList[i]->endWidthIndex = col_end;
        argsStructList[i]->oldPic = (void *)image;
        argsStructList[i]->newPic = (void *)newImage;
        pthread_create(&threads[i], NULL, detectEdgeInParallel, (void *)argsStructList[i]);
    }
    // wait for them to finish and join
    for (int i = 0; i < THREADCOUNT; i++)
    {
        pthread_join(threads[i], NULL);
    }
}

void *detectEdgeInParallel(void *argsArray)
{
    Arguments *args = (Arguments *)argsArray;
    int height = args->height, width = args->width, startWidth = args->startWidthIndex, endWidth = args->endWidthIndex;

    begin = clock();
    RGBTRIPLE(*oldImage)[width] = args->oldPic;
    RGBTRIPLE(*newImage)[width] = args->newPic;

    for (int r = 1; r < height - 1; r++)
    {
        for (int c = startWidth; c < endWidth; c++)
        {
            RGBTRIPLE currPix = oldImage[r][c];
            RGBTRIPLE pix2 = oldImage[r + 1][c];
            RGBTRIPLE pix3 = oldImage[r - 1][c];
            RGBTRIPLE pix4 = oldImage[r][c + 1];
            RGBTRIPLE pix5 = oldImage[r][c - 1];

            int currT = currPix.rgbtRed + currPix.rgbtBlue + currPix.rgbtGreen;
            int t2 = pix2.rgbtRed + pix2.rgbtBlue + pix2.rgbtGreen;
            int t3 = pix3.rgbtRed + pix3.rgbtBlue + pix3.rgbtGreen;
            int t4 = pix4.rgbtRed + pix4.rgbtBlue + pix4.rgbtGreen;
            int t5 = pix5.rgbtRed + pix5.rgbtBlue + pix5.rgbtGreen;

            if (abs(currT - t2) > 100 || abs(currT - t3) > 100 || abs(currT - t4) > 100 || abs(currT - t5) > 100)
            {
                currPix.rgbtRed = 255;
                currPix.rgbtGreen = 255;
                currPix.rgbtBlue = 255;
            }
            else
            {
                currPix.rgbtRed = 0;
                currPix.rgbtGreen = 0;
                currPix.rgbtBlue = 0;
            }
            newImage[r][c] = currPix;
        }
    }

    for (int i = 0; i < height; i++)
    {
        for (int j = startWidth; j < endWidth; j++)
        {
            oldImage[i][j].rgbtRed = newImage[i][j].rgbtRed;
            oldImage[i][j].rgbtGreen = newImage[i][j].rgbtGreen;
            oldImage[i][j].rgbtBlue = newImage[i][j].rgbtBlue;
        }
    }
    end = clock();
    time1 = (double)(end - begin) / CLOCKS_PER_SEC;

    pthread_exit(0);
}

int main(int argc, char **argv)
{

    // begin = clock();
    FILE *inputp = fopen(argv[1], "r");
    if (inputp == NULL)
    {
        fprintf(stderr, "Error: Could not open file");
        return EXIT_FAILURE;
    }

    FILE *outputp = fopen(argv[2], "w");
    if (outputp == NULL)
    {
        fprintf(stderr, "Error: Could not open output file");
        return EXIT_FAILURE;
    }

    BITMAPFILEHEADER b;
    fread(&b, sizeof(BITMAPFILEHEADER), 1, inputp);

    BITMAPINFOHEADER a;
    fread(&a, sizeof(BITMAPINFOHEADER), 1, inputp);

    int height = abs(a.biHeight);
    int width = a.biWidth;

    //Allocating memory for image
    RGBTRIPLE(*image)
    [width] = calloc(height, width * sizeof(RGBTRIPLE));
    if (image == NULL)
    {
        fprintf(stderr, "Not enough memory to store image.\n");
        fclose(outputp);
        fclose(inputp);
        return EXIT_FAILURE;
    }

    // Determine padding for scanlines
    int padding = (4 - (width * sizeof(RGBTRIPLE)) % 4) % 4;

    // Iterate over infile's scanlines
    for (int i = 0; i < height; i++)
    {
        // Read row into pixel array
        fread(image[i], sizeof(RGBTRIPLE), width, inputp);

        // Skip over padding
        fseek(inputp, padding, SEEK_CUR);
    }

    // DETECT EDGES IN PARALLEL
    edgeDetectionParallel(height, width, image);

    fwrite(&b, sizeof(BITMAPFILEHEADER), 1, outputp);
    fwrite(&a, sizeof(BITMAPINFOHEADER), 1, outputp);

    // Write new pixels to outfile
    for (int i = 0; i < height; i++)
    {
        // Write row to outfile
        fwrite(image[i], sizeof(RGBTRIPLE), width, outputp);

        // Write padding at end of row
        for (int k = 0; k < padding; k++)
        {
            fputc(0x00, outputp);
        }
    }
    // end = clock();
    // time1 = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Total CPU time: %e s\n", time1 / THREADCOUNT);
    free(image);
    fclose(inputp);
    fclose(outputp);
    return EXIT_SUCCESS;
}