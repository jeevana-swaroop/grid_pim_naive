#ifndef UTILITY_H
#define UTILITY_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "parm.h"

int importDataset(char *fname, uint64_t N, float *dataset, uint64_t Dim);

void findMaxMin(float *dataset, float *maxArr, float *minArr, uint64_t iDim,
                uint64_t Dim, uint64_t N);

uint64_t getLinearID_nDimensionsGPU(uint64_t *indexes, uint64_t pointIndex, uint64_t *dimLen,
                                    uint64_t nDimensions);

uint64_t findGridIndex(float coordinate, float max, float min,
                       float *epsilon);
void findGridCoordinate(uint64_t *gridCoordinates, pointNId *pointNIdArr,
                        float *dataset, uint64_t pointIndex, float *maxArray,
                        float *minArray, float *epsilon, uint64_t iDim,
                        uint64_t Dim, uint64_t *iDIMLenghts);

int compare(const void *a, const void *b);
int compare_1(const void *a, const void *b);
int compare_2(const void *a, const void *b);

#endif