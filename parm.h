#define DIM 2
#define iDIM 2
#define EPSILON 0.2
#define NR_DPUS 64

typedef struct pointNLinearId
{
    uint64_t pointIndex;
    uint64_t linearCoordinate;
} pointNId;

typedef struct IndexStructure
{

    uint64_t linearCoordinate;
    uint64_t gridCoordinates[iDIM];
    uint64_t startIndex;
    uint64_t endIndex;
    uint64_t count;

} indexStructure;