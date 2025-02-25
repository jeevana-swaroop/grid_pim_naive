#include "utility.h"

int importDataset(char *fname, uint64_t N, float *dataset, uint64_t Dim)
{

  FILE *fp = fopen(fname, "r");

  if (!fp)
  {
    printf("Unable to open file\n");
    return (1);
  }

  char buf[4096];
  uint64_t count = 0;
  uint64_t totalElements = N * Dim;
  while (fgets(buf, 4096, fp) && count < totalElements)
  {
    uint64_t colCnt = 0;
    char *field = strtok(buf, ",");
    float tmp;
    sscanf(field, "%f", &tmp);
    dataset[count] = tmp;

    while (field && (colCnt < Dim - 1))
    {
      colCnt++;
      count++;
      field = strtok(NULL, ",");

      if (field != NULL)
      {
        float tmp;
        sscanf(field, "%f", &tmp);
        dataset[count] = tmp;
      }
    }
    count++;
  }

  fclose(fp);
  return 0;
}

void findMaxMin(float *dataset, float *maxArr, float *minArr, uint64_t iDim,
                uint64_t Dim, uint64_t N)
{

  for (uint64_t i = 0; i < iDim; i++)
  {
    maxArr[i] = dataset[i];
    minArr[i] = dataset[i];
  }

  for (uint64_t i = 0; i < N; i++)
  {

    uint64_t startPoint = i * Dim;

    for (uint64_t j = 0; j < iDim; j++)
    {

      if (dataset[startPoint + j] > maxArr[j])
      {
        maxArr[j] = dataset[startPoint + j];
      }

      if (dataset[startPoint + j] < minArr[j])
      {
        minArr[j] = dataset[startPoint + j];
      }
    }
  }
}

uint64_t getLinearID_nDimensionsGPU(uint64_t *indexes, uint64_t pointIndex,
                                    uint64_t *dimLen, uint64_t nDimensions)
{
  uint64_t offset = 0;
  uint64_t multiplier = 1;
  for (uint64_t i = 0; i < nDimensions; i++)
  {
    offset += indexes[pointIndex * nDimensions + i] * multiplier;
    multiplier *= dimLen[i];
  }

  return offset;
}

uint64_t findGridIndex(float coordinate, float max, float min,
                       float *epsilon)
{

  uint64_t gridIndex = (uint64_t)((coordinate - min) / *epsilon);

  return gridIndex;
}

void findGridCoordinate(uint64_t *gridCoordinates, pointNId *pointNIdArr,
                        float *dataset, uint64_t pointIndex, float *maxArray,
                        float *minArray, float *epsilon, uint64_t iDim,
                        uint64_t Dim, uint64_t *iDIMLenghts)
{

  uint64_t startIndexGridCoordinates = pointIndex * iDim;
  uint64_t startIndexDataSet = pointIndex * Dim;

  for (uint64_t i = 0; i < iDim; i++)
  {

    gridCoordinates[startIndexGridCoordinates + i] = findGridIndex(
        dataset[startIndexDataSet + i], maxArray[i], minArray[i], epsilon);
  }

  pointNIdArr[pointIndex].linearCoordinate = getLinearID_nDimensionsGPU(
      gridCoordinates, pointIndex, iDIMLenghts, iDim);

  pointNIdArr[pointIndex].pointIndex = pointIndex;
}

int compare(const void *a, const void *b)
{
  uint64_t i = (uint64_t)(((pointNId *)a)->linearCoordinate);
  uint64_t j = (uint64_t)(((pointNId *)b)->linearCoordinate);
  return (int)(i - j);
}

int compare_1(const void *a, const void *b)
{
  uint64_t i = (uint64_t)(((indexStructure *)a)->gridCoordinates[0]);
  uint64_t j = (uint64_t)(((indexStructure *)b)->gridCoordinates[0]);
  return (int)(i - j);
}

int compare_2(const void *a, const void *b)
{
  uint64_t i = (uint64_t)(((indexStructure *)a)->linearCoordinate);
  uint64_t j = (uint64_t)(((indexStructure *)b)->linearCoordinate);
  return (int)(i - j);
}
