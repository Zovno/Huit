#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct Pixel
{
    unsigned char Red;
    unsigned char Green;
    unsigned char Blue;
}Pixel;

#pragma pack(push, 1)

typedef struct BMPHeader
{
    unsigned char ID[2];
    unsigned int file_seze;
    unsigned char unused[4];
    unsigned int pixel_offset;
} BMPHeader;

typedef struct DIBHeader
{
    unsigned int header_size;
    unsigned int width;
    unsigned int height;
    unsigned short color_planes;
    unsigned short bits_per_pixel;
    unsigned int comp;
    unsigned int data_size;
    unsigned int pwidth;
    unsigned int pheight;
    unsigned int colors_count;
    unsigned int imp_colors_count;
} DIBHeader;

typedef struct BMPFile
{
    BMPHeader bhdr;
    DIBHeader dhdr;
    Pixel* pixels;
}BMPFile;

#pragma pack(pop)

float powf(float base, float exponent)
{
    if (exponent == 0)
        return 1.0;
    else if (exponent > 0)
    {
        float result = base;
        for (int i = 1; i < exponent; i++)
            result *= base;
        return result;
    }
    else
    {
        float result = 1.0 / base;
        for (int i = -1; i > exponent; i--)
            result /= base;
        return result;
    }
}


BMPFile* loadBMPFile(char* fname)
{
    FILE* fp = fopen(fname, "rb");
    if (!fp)
    {
        printf("Can't load file \'%s\'\n", fname);
        return NULL;
    }

    BMPFile* bmp_file = (BMPFile*)malloc(sizeof(BMPFile));
    fread(&bmp_file->bhdr, sizeof(BMPHeader), 1, fp);
    fread(&bmp_file->dhdr, sizeof(DIBHeader), 1, fp);

    int data_size = bmp_file->dhdr.width * bmp_file->dhdr.height;
    bmp_file->pixels = (Pixel*)malloc(data_size * sizeof(Pixel));

    fseek(fp, bmp_file->bhdr.pixel_offset, SEEK_SET);

    int padding = (4 - ((bmp_file->dhdr.width * 3) % 4)) % 4;
    unsigned char padding_buf[3];

    for (int i = bmp_file->dhdr.height - 1; i >= 0; i--)
    {
        for (int j = 0; j < bmp_file->dhdr.width; j++)
        {
            fread(&bmp_file->pixels[i * bmp_file->dhdr.width + j].Blue, 1, 1, fp);
            fread(&bmp_file->pixels[i * bmp_file->dhdr.width + j].Green, 1, 1, fp);
            fread(&bmp_file->pixels[i * bmp_file->dhdr.width + j].Red, 1, 1, fp);
        }
        fread(padding_buf, 1, padding, fp);
    }

    fclose(fp);
    return bmp_file;
}

void saveBMPFile(char* fname, BMPFile* bmp_file)
{
    FILE* fp = fopen(fname, "wb");
    if (!fp)
    {
        printf("Can't create file \'%s\'\n", fname);
        return;
    }

    fwrite(&bmp_file->bhdr, sizeof(BMPHeader), 1, fp);
    fwrite(&bmp_file->dhdr, sizeof(DIBHeader), 1, fp);

    int padding = (4 - ((bmp_file->dhdr.width * 3) % 4)) % 4;
    unsigned char padding_buf[3] = { 0 };

    for (int i = bmp_file->dhdr.height - 1; i >= 0; i--)
    {
        for (int j = 0; j < bmp_file->dhdr.width; j++)
        {
            fwrite(&bmp_file->pixels[i * bmp_file->dhdr.width + j].Blue, 1, 1, fp);
            fwrite(&bmp_file->pixels[i * bmp_file->dhdr.width + j].Green, 1, 1, fp);
            fwrite(&bmp_file->pixels[i * bmp_file->dhdr.width + j].Red, 1, 1, fp);
        }
        fwrite(padding_buf, 1, padding, fp);
    }

    fclose(fp);
}


void convertToNegative(BMPFile* bmp_file)
{
    int pixel_count = bmp_file->dhdr.width * bmp_file->dhdr.height;
    for (int i = 0; i < pixel_count; i++)
    {
        bmp_file->pixels[i].Blue = 255 - bmp_file->pixels[i].Blue;
        bmp_file->pixels[i].Green = 255 - bmp_file->pixels[i].Green;
        bmp_file->pixels[i].Red = 255 - bmp_file->pixels[i].Red;
    }
}


void convertToGrayscale(BMPFile* bmp_file)
{
    int pixel_count = bmp_file->dhdr.width * bmp_file->dhdr.height;
    for (int i = 0; i < pixel_count; i++)
    {
        unsigned char average = (bmp_file->pixels[i].Blue + bmp_file->pixels[i].Green + bmp_file->pixels[i].Red) / 3;
        bmp_file->pixels[i].Blue = average;
        bmp_file->pixels[i].Green = average;
        bmp_file->pixels[i].Red = average;
    }
}


void applyMedianFilter(BMPFile* bmp_file, int filter_size)
{
    int width = bmp_file->dhdr.width;
    int height = bmp_file->dhdr.height;
    int half_filter_size = filter_size / 2;


    Pixel* temp_pixels = (Pixel*)malloc(width * height * sizeof(Pixel));
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            temp_pixels[i * width + j] = bmp_file->pixels[i * width + j];
        }
    }

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {

            Pixel* filter = (Pixel*)malloc(filter_size * filter_size * sizeof(Pixel));
            int index = 0;
            for (int m = -half_filter_size; m <= half_filter_size; m++)
            {
                for (int n = -half_filter_size; n <= half_filter_size; n++)
                {
                    int row = i + m;
                    int col = j + n;
                    if (row < 0) row = 0;
                    if (row >= height) row = height - 1;
                    if (col < 0) col = 0;
                    if (col >= width) col = width - 1;
                    filter[index++] = temp_pixels[row * width + col];
                }
            }


            for (int k = 0; k < filter_size * filter_size - 1; k++)
            {
                for (int l = 0; l < filter_size * filter_size - k - 1; l++)
                {
                    int intensity1 = filter[l].Blue + filter[l].Green + filter[l].Red;
                    int intensity2 = filter[l + 1].Blue + filter[l + 1].Green + filter[l + 1].Red;
                    if (intensity1 > intensity2)
                    {
                        Pixel temp = filter[l];
                        filter[l] = filter[l + 1];
                        filter[l + 1] = temp;
                    }
                }
            }


            int median_index = (filter_size * filter_size) / 2;
            bmp_file->pixels[i * width + j] = filter[median_index];

            free(filter);
        }
    }

    free(temp_pixels);
}


unsigned char applyGamma(unsigned char value, float gammaInv)
{
    float normalizedValue = value / 255.0;
    float correctedValue = powf(normalizedValue, gammaInv) * 255.0;
    return (unsigned char)(correctedValue + 0.5); 
}



void applyGammaCorrection(BMPFile* bmp_file, float gamma)
{
    int width = bmp_file->dhdr.width;
    int height = bmp_file->dhdr.height;

    float gammaInv = 1.0 / gamma;

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            Pixel* pixel = &bmp_file->pixels[i * width + j];

            pixel->Red = applyGamma(pixel->Red, gammaInv);
            pixel->Green = applyGamma(pixel->Green, gammaInv);
            pixel->Blue = applyGamma(pixel->Blue, gammaInv);
        }
    }
}

bool checkImg(const char* filename) {
    const char* extension = strrchr(filename, '.');
    if (extension == NULL || strcmp(extension, ".bmp") != 0) {
        printf("Wrong format file\n");
        return 0;
    }

    return 1;
}

int menu()
{
    printf("/----------------------\\\n");
    printf("| 1) Gamma correction   |\n");
    printf("| 2) Median filter      |\n");
    printf("| 3) To grayscale       |\n");
    printf("| 4) To negative        |\n");
    printf("| 0) exit               |\n");
    printf("\\______________________/\n");
    printf("Ur choise: ");
    int choise;
    scanf("%d", &choise);
    return choise;
}

void saveBMP(BMPFile* bmpf)
{
    char filename[100];

    printf("Enter new file name: ");
    scanf("%s", filename);

    if (checkImg(filename))
    {
        saveBMPFile(filename, bmpf);
        return;
    }

    printf("Error");
}

int main()
{
    char filename[100];
    int a;

    while (true)
    {
        printf("Enter file name: ");
        scanf("%s", filename);

        if (checkImg(filename))
        {
            BMPFile* bmpf = loadBMPFile(filename);

            switch (menu())
            {
            case 1:
                printf("\n Enter gamma (0-255): ");
                scanf("%d", &a);
                applyGammaCorrection(bmpf, a);
                saveBMP(bmpf);
                break;
            case 2: 
                printf("\n Enter filtersize (x%%2==1): ");
                scanf("%d", &a);
                applyMedianFilter(bmpf, a);
                saveBMP(bmpf);
                break;
            case 3: 
                convertToGrayscale(bmpf);
                saveBMP(bmpf);
                break;
            case 4: 
                convertToNegative(bmpf);
                saveBMP(bmpf);
                break;
            case 0: return 0;
            default:
                break;
            }
        }
    }
    return 0;
}