#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#define _USE_MATH_DEFINES
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
	unsigned char** r;
	unsigned char** g;
	unsigned char** b;
} Channels;

void apply_filter_to_pixel(unsigned char** r, unsigned char** g, unsigned char** b, int x, int y, float** filter, unsigned char** r2, unsigned char** g2, unsigned char** b2);
void free_pixel_array(unsigned char** array, int height);
void free_filter(float** filter, int filter_size);
float** create_filter(int filter_size);
Channels create_rgb_channels(int height, int width);
void extract_color_channels(Channels* channels_struct, int height, int width, int channels, unsigned char* image);
void apply_filter(Channels channels1, Channels channels2, float** filter, int new_height, int new_width);
void free_channels(Channels channels, int height);
void channels_to_array(unsigned char* arr, Channels* channels_struct, int new_height, int new_width, int channels);
void brightness_boost(Channels channels, int height, int width);

int main() {
	int width, height, channels;
	unsigned char* image = stbi_load("image.png", &width, &height, &channels, 0);
	if (image == NULL) {
		printf("Error: Image not found\n");
		return 1;
	}
	printf("Image loaded successfully\n");
	printf("Width: %d\nHeight: %d\nChannels: %d\n", width, height, channels);
	// print a few pixels to check if the image is loaded correctly
	for (int i = 0; i < 10; i++) {
		printf("Pixel %d: R:%d G:%d B:%d\n", i, image[i * channels], image[i * channels + 1], image[i * channels + 2]);
	}

	// Allocate memory to store the 3 color channels
	Channels channels_struct = create_rgb_channels(height, width);

	// Extract the 3 color channels from the image
	extract_color_channels(&channels_struct, height, width, channels, image);
	unsigned char** r = channels_struct.r;
	unsigned char** g = channels_struct.g;
	unsigned char** b = channels_struct.b;

	// Print the RGB at pixel position (100, 100)
	if (width > 100 && height > 100)
		printf("Pixel (100, 100): R:%d G:%d B:%d\n", r[100][100], g[100][100], b[100][100]);

	// Create filter
	int filter_size = 5;
	float** filter = create_filter(filter_size);

	int new_width = width - filter_size + 1;
	int new_height = height - filter_size + 1;
	Channels channels_struct2 = create_rgb_channels(new_height, new_width);
	unsigned char** r2 = channels_struct2.r;
	unsigned char** g2 = channels_struct2.g;
	unsigned char** b2 = channels_struct2.b;

	// Apply filter to the image
	apply_filter(channels_struct, channels_struct2, filter, new_height, new_width);

	// Contrast boost
	brightness_boost(channels_struct2, new_height, new_width);

	// Print the RGB at pixel position (100, 100) after applying the filter
	if (new_width > 100 && new_height > 100)
		printf("Pixel (100, 100) after applying the filter: R:%d G:%d B:%d\n", r2[100][100], g2[100][100], b2[100][100]);

	// print a few pixels of the denoised image
	if (new_width > 10 && new_height > 10) {
		printf("Denoised Pixels:\n");
		for (int i = 0; i < 10; i++) {
			printf("Denoised Pixel %d: R:%d G:%d B:%d\n", i, r2[i][i], g2[i][i], b2[i][i]);
		}
	}

	// Convert back to 1D array
	unsigned char* denoised_image = malloc(new_width * new_height * channels * sizeof(unsigned char));
	if (denoised_image == NULL) {
		printf("Error: Memory allocation failed\n");
		return 1;
	}
	channels_to_array(denoised_image, &channels_struct2, new_height, new_width, channels);
	
	stbi_write_png("denoised_image.png", new_width, new_height, channels, denoised_image, new_width * channels);

	stbi_image_free(image);
	free_channels(channels_struct, height);
	free_channels(channels_struct2, new_height);
	free_filter(filter, filter_size);
	free(denoised_image);
	return 0;
}

void apply_filter_to_pixel(unsigned char** r, unsigned char** g, unsigned char** b, int x, int y, float** filter, unsigned char** r2, unsigned char** g2, unsigned char** b2) {
	float sum_r = 0;
	float sum_g = 0;
	float sum_b = 0;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			sum_r += r[y + i][x + j] * filter[i][j];
			sum_g += g[y + i][x + j] * filter[i][j];
			sum_b += b[y + i][x + j] * filter[i][j];
		}
	}
	r2[y][x] = sum_r;
	g2[y][x] = sum_g;
	b2[y][x] = sum_b;
}

void free_pixel_array(unsigned char** array, int height) {
	for (int i = 0; i < height; i++) {
		free(array[i]);
	}
	free(array);
}

float** create_filter(int filter_size) {
	float** filter = malloc(filter_size * sizeof(float*));
	if (filter == NULL) {
		printf("Error: Memory allocation failed\n");
		exit(1);
	}
	for (int i = 0; i < filter_size; i++) {
		filter[i] = malloc(filter_size * sizeof(float));
		if (filter[i] == NULL) {
			printf("Error: Memory allocation failed\n");
			exit(1);
		}
	}
	// Simple averaging filter
	/*for (int i = 0; i < filter_size; i++) {
		for (int j = 0; j < filter_size; j++) {
			filter[i][j] = 1.0 / (filter_size * filter_size);
		}
	}*/

	// Gaussian filter
	float sigma = 5.0;
	float sum = 0.0f;
	int center = filter_size / 2;

	for (int i = 0; i < filter_size; i++) {
		for (int j = 0; j < filter_size; j++) {
			int x = i - center;
			int y = j - center;
			filter[i][j] = exp(-(x * x + y * y) / (2 * sigma * sigma)) / (2 * M_PI * sigma * sigma);
			sum += filter[i][j];
		}
	}

	// Normalize the filter
	for (int i = 0; i < filter_size; i++) {
		for (int j = 0; j < filter_size; j++) {
			filter[i][j] /= sum;
		}
	}

	return filter;
}

void free_filter(float** filter, int filter_size) {
	for (int i = 0; i < filter_size; i++) {
		free(filter[i]);
	}
	free(filter);
}

Channels create_rgb_channels(int height, int width) {
	Channels channels;
	channels.r = malloc(height * sizeof(unsigned char*));
	channels.g = malloc(height * sizeof(unsigned char*));
	channels.b = malloc(height * sizeof(unsigned char*));
	if (channels.r == NULL || channels.g == NULL || channels.b == NULL) {
		printf("Error: Memory allocation failed\n");
		exit(1);
	}
	for (int i = 0; i < height; i++) {
		channels.r[i] = malloc(width * sizeof(unsigned char));
		channels.g[i] = malloc(width * sizeof(unsigned char));
		channels.b[i] = malloc(width * sizeof(unsigned char));
		if (channels.r[i] == NULL || channels.g[i] == NULL || channels.b[i] == NULL) {
			printf("Error: Memory allocation failed\n");
			exit(1);
		}
	}
	return channels;
}

void free_channels(Channels channels, int height) {
	for (int i = 0; i < height; i++) {
		free(channels.r[i]);
		free(channels.g[i]);
		free(channels.b[i]);
	}
	free(channels.r);
	free(channels.g);
	free(channels.b);
}

void extract_color_channels(Channels* channels_struct, int height, int width, int channels, unsigned char* image) {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int index = (y * width + x) * channels;
			channels_struct->r[y][x] = image[index];
			channels_struct->g[y][x] = image[index + 1];
			channels_struct->b[y][x] = image[index + 2];
		}
	}
}

void apply_filter(Channels channels1, Channels channels2, float** filter, int new_height, int new_width) {
	for (int y = 0; y < new_height; y++) {
		for (int x = 0; x < new_width; x++) {
			apply_filter_to_pixel(channels1.r, channels1.g, channels1.b, x, y, filter, channels2.r, channels2.g, channels2.b);
		}
	}
}

void channels_to_array(unsigned char* arr, Channels* channels_struct, int new_height, int new_width, int channels) {
	for (int y = 0; y < new_height; y++) {
		for (int x = 0; x < new_width; x++) {
			int index = (y * new_width + x) * channels;
			arr[index] = channels_struct->r[y][x];
			arr[index + 1] = channels_struct->g[y][x];
			arr[index + 2] = channels_struct->b[y][x];
		}
	}
}

void brightness_boost(Channels channels, int height, int width) {
	float factor = 2.5;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			channels.r[y][x] = fmin(channels.r[y][x] * factor, 255);
			channels.g[y][x] = fmin(channels.g[y][x] * factor, 255);
			channels.b[y][x] = fmin(channels.b[y][x] * factor, 255);
		}
	}
}