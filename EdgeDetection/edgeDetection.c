#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "bmp.h"

double time1;
clock_t begin, end;

void edgeDetection(int height, int width, RGBTRIPLE (*image)[width])
{
    begin = clock();
    RGBTRIPLE newImage[height][width];

    for (int r = 1; r < height - 1; r++)
    {
        for (int c = 1; c < width - 1; c++)
        {
            RGBTRIPLE currPix = image[r][c];
            RGBTRIPLE pix2 = image[r + 1][c];
            RGBTRIPLE pix3 = image[r - 1][c];
            RGBTRIPLE pix4 = image[r][c + 1];
            RGBTRIPLE pix5 = image[r][c - 1];

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
        for (int j = 0; j < width; j++)
        {
            image[i][j].rgbtRed = newImage[i][j].rgbtRed;
            image[i][j].rgbtGreen = newImage[i][j].rgbtGreen;
            image[i][j].rgbtBlue = newImage[i][j].rgbtBlue;
        }
    }

    end = clock();
    time1 = (double)(end - begin) / CLOCKS_PER_SEC;
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

    // DETECT EDGES
    edgeDetection(height, width, image);

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
    printf("Total CPU time: %e s\n", time1);
    free(image);
    fclose(inputp);
    fclose(outputp);
    return EXIT_SUCCESS;
}