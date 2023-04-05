#ifndef _TEACHING_TYPES_HH
#define _TEACHING_TYPES_HH

#include <limits>
#include <vector>

namespace Teaching {
typedef unsigned int Time;
typedef unsigned short Mixture;//every arc has 6 states? Mixture=0,1,2,3,4,5?
typedef unsigned int Word;
typedef unsigned short Phoneme;
typedef unsigned short State;
typedef unsigned int Index;
typedef std::vector<Word> WordSequence;
typedef std::vector<Mixture> MixtureSequence;
typedef float Score;

static const Word invalidWord = std::numeric_limits<Word>::max();
static const Index invalidIndex = std::numeric_limits<Index>::max();
static const Score maxScore = std::numeric_limits<Score>::max();//what's this maxScore? Threshold?
} // namespace Teaching

#endif // _TEACHING_TYPES_HH
