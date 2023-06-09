#include "WordConditionedTreeSearch.hh"
#include "BookKeeping.hh"
#include <algorithm>
#include <map>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

using namespace Teaching;

// -------------------------------------------------------------------------------------------------

/**
 * Tree lexicon for word-conditioned tree search. Constructs tree lexicon and
 * provides access methods.
 */
class TreeLexicon {
private:
  /** array version of tree lexicon */
  struct TreeLexiconArc {
    /** first successor arc */
    Arc succArcBegin;

    /** last (exclusive) successor arc */
    Arc succArcEnd;

    /** ending word at this arc */
    Word endingWord;

    /** mixtures of arc */
    MixtureSequence mixtures;//what's this mixtures?

    TreeLexiconArc(Arc succArcBegin, Arc succArcEnd, Word endingWord,
                   const MixtureSequence &mixtures)
        : succArcBegin(succArcBegin), succArcEnd(succArcEnd),
          endingWord(endingWord), mixtures(mixtures) {}
  };

  typedef std::vector<TreeLexiconArc> TreeLexiconArcs;
  TreeLexiconArcs treeLexiconArcs_;

  Word nWords_;

  const Word silence_;

  const Mixture silenceMixture_;

public:
  TreeLexicon(const Lexicon &lexicon);

  /**
   * Return word that ends at given arc.
   */
  inline Word endingWord(Arc arc) const {
    require(arc >= 0 && arc < nArcs());
    return treeLexiconArcs_[arc].endingWord;
  }

  /**
   * Return mixture sequence belonging to given arc.
   */
  inline const MixtureSequence &mixtures(Arc arc) const {
    require(arc >= 0 && arc < nArcs());
    return treeLexiconArcs_[arc].mixtures;
  }

  /**
   * Return first (inclusive) successor arc of given arc.
   */
  inline Arc succArcBegin(Arc arc) const {
    require(arc >= 0 && arc < nArcs());
    return treeLexiconArcs_[arc].succArcBegin;
  }

  /**
   * Return last (exclusive) successor arc of given arc.
   */
  inline Arc succArcEnd(Arc arc) const {
    require(arc >= 0 && arc < nArcs());
    return treeLexiconArcs_[arc].succArcEnd;
  }

  /**
   * Return number of states of given arc.
   */
  inline HmmState nStates(Arc arc) const {
    require(arc >= 0 && arc < nArcs());
    return treeLexiconArcs_[arc].mixtures.size();
  }

  /**
   * Return number of words in lexicon.
   */
  inline Word nWords() const { return nWords_; }

  /**
   * Return total number of arcs in tree lexicon.
   */
  inline Arc nArcs() const { return treeLexiconArcs_.size(); }

  /**
   * Return silence word.
   */
  inline Word silence() const { return silence_; }

  /**
   * Return silence mixture id (silence phoneme is modelled by
   * only one state).
   */
  inline Mixture silenceMixture() const { return silenceMixture_; }
};

struct TreeNode {
  std::map<std::string, TreeNode> children;
  Word endingWord;
  Word word;
  size_t depth = 0;
  std::string mark;
};

TreeLexicon::TreeLexicon(const Lexicon &lexicon)//lexicon tree for words(LM)?
    : nWords_(lexicon.nWords()), silence_(lexicon.silence()),
      silenceMixture_(lexicon.silenceMixture()) {
    //Sheet 6 Task 1 Prefix Tree
    //Can we also write nWords=lexicon.nWords(), silence_=lexicon.silence(), lexicon.silenceMixture_=lexicon.silenceMixture() here?

  TreeNode root;
  root.mark = "0";
  size_t nodeid = 1;
  for (Word w = 0; w < nWords_; ++w) {
    std::cout << "Word " << w << " " << lexicon.symbol(w) << std::endl;
    TreeNode *currentNode = &root;

    for (int p = 0; p < lexicon.nPhonemes(w); ++p) {
      std::string curAllo = lexicon.format(*lexicon.allophone(w, p));
      std::map<std::string, TreeNode>::iterator it = currentNode->children.find(curAllo);
      if (it == currentNode->children.end()) {
        TreeNode childNode;
        childNode.endingWord = lexicon.nWords();//endingWord=number of words?
        childNode.word = w;
        childNode.depth = currentNode->depth + 1;
        {
          std::stringstream ss;//what's this stringstream for? Can we directly childNode.mark=nodeid++.str();?
          ss << nodeid++;
          childNode.mark = ss.str();
        }
        currentNode->children[lexicon.format(*lexicon.allophone(w, p))] = childNode;
      }
      currentNode = &currentNode->children[curAllo];
      std::cout << curAllo << " ";
    }
    currentNode->word = w;
    currentNode->endingWord = w;
    currentNode->mark = "\"" + lexicon.symbol(w) + "\"";
    std::cout << std::endl;
  }

  std::queue<TreeNode *> queue;
  MixtureSequence mixtures;//mixtures stores the states of an arc?
  size_t last_arc = 1;
  queue.push(&root);

  while (!queue.empty()) {
    TreeNode &currentNode = *queue.front();
    size_t numChild = currentNode.children.size();
    MixtureSequence mixtures;
    if (currentNode.depth > 0) {
        mixtures = *lexicon.mixtures(currentNode.word, currentNode.depth - 1);
        treeLexiconArcs_.push_back(
                TreeLexiconArc(last_arc, last_arc + numChild, currentNode.endingWord, mixtures));
    }
    last_arc += numChild;
    for (std::map<std::string, TreeNode>::iterator child = currentNode.children.begin(); child != currentNode.children.end(); child++) {
      queue.push(&child->second);
    }
    queue.pop();
  }

  std::cout << "digraph tree {" << std::endl;
  {
    std::stringstream nodes;
    std::stringstream edges;
    std::queue<TreeNode *> queue;
    {
      queue.push(&root);
      size_t i = 0;
      while (!queue.empty()) {
        TreeNode &currentNode = *queue.front();
        nodes << currentNode.mark << std::endl;
        for (std::map<std::string, TreeNode>::iterator child =
                 currentNode.children.begin();
             child != currentNode.children.end(); child++) {
          queue.push(&child->second);
          edges << currentNode.mark << " -> " << child->second.mark;
            edges << " [label=\"" << child->first << "\"];" << std::endl;
        }
        queue.pop();
      }
    }
    std::cout << nodes.str() << edges.str();
  }
  std::cout << "}";
}

// -------------------------------------------------------------------------------------------------

/**
 * Stores for every tree, conditioned on predecessor word, if it is active and
 * if it needs to be restarted based on a given word hypothesis.
 */
class WordConditionedTreeSearch::ActiveTrees {
private:
  struct Tree {
  public:
    /** true if tree is active, false otherwise */
    bool isActive;

    /** word hypothesis starting a new tree */
    const WordHypothesis *wordHyp;

    Tree(bool isActive, const WordHypothesis *wordHyp = 0)
        : isActive(isActive), wordHyp(wordHyp) {}
  };

  typedef std::vector<Tree> TreeMap;
  TreeMap treeMap_;

public:
  ActiveTrees(Word nWords);

  /**
   * Clear map so that there are no active arcs.
   */
  void clear();

  /**
   * Return true if tree conditioned on given word is active, false otherwise.
   */
  bool isActive(Word predecessorWord) const;

  /**
   * Set tree to active.
   */
  void setActive(Word predecessorWord);

  /**
   * Set tree to inactive.
   */
  void setInactive(Word predecessorWord);

  /**
   * Set tree to active and mark it to be restarted with initial
   * score/backpointer from given word hypothesis.
   */
  void restartTree(Word predecessorWord, const WordHypothesis *wordHyp);

  /**
   * Return the word hypothesis that will restart the tree conditioned on given
   * word.
   */
  const WordHypothesis *wordHyp(Word predecessorWord) const;
};

WordConditionedTreeSearch::ActiveTrees::ActiveTrees(Word nWords) {
  treeMap_.resize(nWords, Tree(false));
}

void WordConditionedTreeSearch::ActiveTrees::clear() {
  std::fill(treeMap_.begin(), treeMap_.end(), Tree(false));
}

inline bool
WordConditionedTreeSearch::ActiveTrees::isActive(Word predecessorWord) const {
  require(predecessorWord >= 0 && predecessorWord < treeMap_.size());
  return treeMap_[predecessorWord].isActive;
}

inline void
WordConditionedTreeSearch::ActiveTrees::setActive(Word predecessorWord) {
  require(predecessorWord >= 0 && predecessorWord < treeMap_.size());
  treeMap_[predecessorWord] = Tree(true);
}

inline void
WordConditionedTreeSearch::ActiveTrees::setInactive(Word predecessorWord) {
  require(predecessorWord >= 0 && predecessorWord < treeMap_.size());
  treeMap_[predecessorWord] = Tree(false);
}

inline void WordConditionedTreeSearch::ActiveTrees::restartTree(
    Word predecessorWord, const WordHypothesis *wordHyp) {
  require(predecessorWord >= 0 && predecessorWord < treeMap_.size());
  treeMap_[predecessorWord] = Tree(true, wordHyp);
}

inline const WordConditionedTreeSearch::WordHypothesis *
WordConditionedTreeSearch::ActiveTrees::wordHyp(Word predecessorWord) const {
  require(predecessorWord >= 0 && predecessorWord < treeMap_.size());
  return treeMap_[predecessorWord].wordHyp;
}

// -------------------------------------------------------------------------------------------------

struct ArcPredecessors {
  /** index into vector of arc hypotheses */
  Index arcHypIndex;

  /** direct predecessor state of arc */
  HmmState predecessor;

  /** pre-predecessor state of arc */
  HmmState prePredecessor;

  /** score of predecessor state of arc */
  Score predecessorScore;

  /** score of pre-predecessor state of arc */
  Score prePredecessorScore;

  /** backpointer of predecessor state of arc */
  Index predecessorBackpointer;

  /** backpointer of pre-predecessor state of arc */
  Index prePredecessorBackpointer;

  /** true if arc is (re-)started, false otherwise */
  bool isStarted;

  ArcPredecessors(Index arcHypIndex, HmmState predecessor,
                  HmmState prePredecessor)
      : arcHypIndex(arcHypIndex), predecessor(predecessor),
        prePredecessor(prePredecessor), predecessorScore(maxScore),
        prePredecessorScore(maxScore), predecessorBackpointer(invalidIndex),
        prePredecessorBackpointer(invalidIndex), isStarted(false) {}
};

/**
 * Search space of word-conditioned tree search: Store and manage active
 * tree/arc/state/word hypotheses.
 */
class WordConditionedTreeSearch::SearchSpace {
public:
  /**
   * Initialize search space.
   */
  SearchSpace(const Lexicon &lexicon);

  /**
   * Destructor for search space: delete data.
   */
  virtual ~SearchSpace();

  /**
   * Insert initial silence hypothesis.
   */
  void addInitialHypothesis();

  /**
   * Initialize search space for every speech segment.
   */
  void reset();

  /**
   * Expand all state hypothesis by one time frame.
   */
  void expand(Time time, Score acousticPruningThreshold,
              AcousticModelScorer &amScorer);

  /**
   * Convert word hypotheses (from wordHypotheses_) into new trees (in
   * treeHypotheses_), conditioned on these words.
   */
  void startNewTrees();

  /**
   * Store dynamic programming history in book keeping.
   */
  void addBookKeepingEntries(Time time);

  /**
   * Discard hypotheses that do not fulfill the acoustic pruning criterion, and
   * find word end hypotheses.
   */
  void pruneStatesAndFindWordEnds(Score acoustigPruningThreshold);

  /**
   * Iterate over word hypotheses, add bigram language model scores and do
   * recombination (only keep the best word hypothesis for a specific word
   * and arbitrary predecessor word).
   */
  void bigramRecombination(const LanguageModelScorer &lmScore);

  /**
   * Trace back word sequence in book keeping.
   */
  void traceback(Traceback &result) const;

  /**
   * Set time distortion penalties.
   */
  void setTransitionScores(const TransitionModelScorer &transitionScore);

  /**
   * Return current best score of all state hypotheses.
   */
  Score currentBestScore() const { return bestScore_; }

  /**
   * Return current best score of all word hypotheses.
   */
  Score currentBestWordHypothesisScore() const {
    if (wordHypotheses_.empty())
      return maxScore;
    return min_element(wordHypotheses_.begin(), wordHypotheses_.end())->score;
  }

  /**
   * Return true if arc is within the first generation of the tree lexicon,
   * otherwise return false.
   */
  inline bool isFirstGenerationArc(Arc arc) const {
    require(arc >= 0 && arc < treeLexicon_.nArcs());
    return arc < treeLexicon_.succArcEnd(0);
  }

private:
  class ActiveArcs;

  typedef std::vector<Index>::const_iterator ArcIndexIterator;

  struct TreeHypothesis {
    /** predecessor word of tree hypothesis */
    Word predecessorWord;

    /** first (inclusive) and last (exclusive) arc hypothesis associated with
     * this tree */
    Index arcHypBegin, arcHypEnd;

    TreeHypothesis(Word predecessorWord, Index arcHypBegin, Index arcHypEnd)
        : predecessorWord(predecessorWord), arcHypBegin(arcHypBegin),
          arcHypEnd(arcHypEnd) {}
  };

  struct ArcHypothesis {
    /** arc id */
    Arc arc;

    /** first (inclusive) and last (exclusive) state hypothesis associated with
     * this arc */
    Index stateHypBegin, stateHypEnd;

    ArcHypothesis(Arc arc, Index stateHypBegin, Index stateHypEnd)
        : arc(arc), stateHypBegin(stateHypBegin), stateHypEnd(stateHypEnd) {
      require(stateHypBegin <= stateHypEnd);
    }
  };

  struct StateHypothesis {
    /** HMM state */
    HmmState state;

    /** score of state hypothesis */
    Score score;

    /** backpointer of state hypothesis */
    Index backpointer;

    StateHypothesis(HmmState state, Score score, Index backpointer)
        : state(state), score(score), backpointer(backpointer) {}

    bool operator<(const StateHypothesis &other) const {
      return score < other.score;
    }
  };

  typedef std::vector<TreeHypothesis> TreeHypotheses;
  typedef std::vector<ArcHypothesis> ArcHypotheses;
  typedef std::vector<StateHypothesis> StateHypotheses;
  typedef std::vector<WordHypothesis> WordHypotheses;

  struct Node {
    Score score;
    Index backpointer;
  };

  /** maximum allowed number of states that may be skipped at once */
  static const HmmState maxSkip_;

  /** garbage collection interval for book keeping */
  static const Time purgeStorageInterval_;

  const TreeLexicon treeLexicon_;
  ActiveTrees activeTrees_;
  ActiveArcs *activeArcs_;
  BookKeeping *bookKeeping_;

  /** time distortion penalties */
  typedef std::vector<std::vector<Score> > Tdps;
  Tdps transitionScores_;

  TreeHypotheses treeHypotheses_;
  TreeHypotheses newTreeHypotheses_;
  ArcHypotheses arcHypotheses_;
  ArcHypotheses newArcHypotheses_;
  StateHypotheses stateHypotheses_;
  StateHypotheses newStateHypotheses_;
  WordHypotheses wordHypotheses_;

  HmmState bestState_;
  Arc bestArc_;
  Score bestScore_;

  /**
   * Return true if given mixture is silence, false otherwise.
   */
  bool isSilence(const Mixture mixture) const;

  /**
   * Store newly created arc hypothesis.
   */
  void writeBackArcHypothesis(Arc arc);

  /**
   * Store newly created tree hypothesis.
   */
  void writeBackTreeHypothesis(TreeHypothesis treeHyp);

  /**
   * Return first predecessor word of given backpointer that is not silence, or
   * silence if there are no non-silence predecessors.
   */
  inline Word nonSilencePredecessorWord(Index backpointer);

  /**
   * Compute time alignment for current time frame.
   */
  void computeTimeAlignment(const Arc arc, const AcousticModelScorer &amScorer,
                            Score amThreshold);

  /**
   * Initialize time alignment.
   */
  void initTimeAlignment(Node nodes[], HmmState &maxState, const Arc arc,
                         const HmmState nStates);
};

// -------------------------------------------------------------------------------------------------

/**
 * Creates a map that holds the predecessor state information of the active arcs
 * of a given tree hypothesis.
 */
class WordConditionedTreeSearch::SearchSpace::ActiveArcs {
private:
  std::vector<Index> activeArcs_;
  std::vector<ArcPredecessors> predecessorMap_;

  const TreeLexicon &treeLexicon_;
  const StateHypotheses &stateHyps_;
  const ArcHypotheses &arcHyps_;

  /**
   * Create map that stores for every arc its predecessor hmm states.
   */
  void initPredecessorMap(const TreeHypothesis &treeHyp);

  /**
   * Activate first generation arcs if tree needs to be restarted.
   */
  void activateFirstGenerationArcs(const WordHypothesis *wordHyp,
                                   const Score bestScore,
                                   const Score amThreshold);

  /**
   * Within-word arcs: activate arcs reachable from previously active arcs.
   */
  void activateWithinWordArcs(const TreeHypothesis &treeHyp,
                              const Score amThreshold, const Score bestScore);

public:
  /**
   * Create map that stores for every arc its predecessor hmm states given a
   * tree hypothesis.
   */
  ActiveArcs(const TreeLexicon &treeLexicon, const StateHypotheses &stateHyps,
             const ArcHypotheses &arcHyps);

  void initialize(const TreeHypothesis &treeHyp, const WordHypothesis *wordHyp,
                  const Score bestScore, const Score amThreshold);

  /**
   * Iterator for iterating over all active arcs of the current tree.
   */
  std::vector<Index>::const_iterator begin() const;

  /**
   * End of active arc iterator.
   */
  std::vector<Index>::const_iterator end() const;

  /**
   * Return predecessor HMM state of given arc.
   */
  HmmState predecessor(Arc arc) const;

  /**
   * Return pre-predecessor HMM state of given arc.
   */
  HmmState prePredecessor(Arc arc) const;

  /**
   * Return index of given arc in vector of arc hypothesis.
   */
  Index arcHypIndex(Arc arc) const;

  /**
   * Return if the arc is re-started.
   */
  bool isStarted(const Arc arc) const;

  /**
   * Return score of predecessor HMM state.
   */
  Score predecessorScore(Arc arc) const;

  /**
   * Return score of pre-predecessor HMM state.
   */
  Score prePredecessorScore(Arc arc) const;

  /**
   * Return backpointer of predecessor HMM state.
   */
  Index predecessorBackpointer(Arc arc) const;

  /**
   * Return backpointer of pre-predecessor HMM state.
   */
  Index prePredecessorBackpointer(Arc arc) const;

  /**
   * Return true if arc has active within-arc HMM state hypotheses.
   */
  bool hasActiveWithinArcStates(Arc arc) const;
};

// -------------------------------------------------------------------------------------------------

WordConditionedTreeSearch::SearchSpace::ActiveArcs::ActiveArcs(
    const TreeLexicon &treeLexicon, const StateHypotheses &stateHyps,
    const ArcHypotheses &arcHyps)
    : treeLexicon_(treeLexicon), stateHyps_(stateHyps), arcHyps_(arcHyps) {}

// Set_ArcArrays
void WordConditionedTreeSearch::SearchSpace::ActiveArcs::initPredecessorMap(
    const TreeHypothesis &treeHyp) {
  activeArcs_.clear();
  predecessorMap_.clear();
  predecessorMap_.resize(
      treeLexicon_.nArcs(),
      ArcPredecessors(invalidIndex, inactiveState, inactiveState));

  const ArcHypotheses::const_iterator arcHypBegin = arcHyps_.begin();
  for (ArcHypotheses::const_iterator arcHyp = arcHypBegin + treeHyp.arcHypBegin;
       arcHyp != arcHypBegin + treeHyp.arcHypEnd; ++arcHyp) {
    const Arc arc = arcHyp->arc;
    require(arcHyp - arcHypBegin >= 0 &&
            arcHyp - arcHypBegin < (int)arcHyps_.size());
    predecessorMap_[arc] =
        ArcPredecessors(arcHyp - arcHypBegin, inactiveState, inactiveState);
    //        require(std::find(activeArcs_.begin(), activeArcs_.end(), arc) ==
    //        activeArcs_.end());
    require(arc < treeLexicon_.nArcs() || arc == invalidArc);
    activeArcs_.push_back(arc);
  }
}

// Set_WordLinks
void WordConditionedTreeSearch::SearchSpace::ActiveArcs::
    activateFirstGenerationArcs(const WordHypothesis *wordHyp,
                                const Score bestScore,
                                const Score amThreshold) {
  // tree shall be activated with initial score
  if (wordHyp != 0) {
    const Score initialScore = wordHyp->score;

    for (Arc successorArc = treeLexicon_.succArcBegin(0);
         successorArc != treeLexicon_.succArcEnd(0); ++successorArc) {
      if (amThreshold == maxScore || bestScore == maxScore ||
          initialScore < bestScore + amThreshold) {
        predecessorMap_[successorArc].isStarted = true;
        predecessorMap_[successorArc].predecessorScore = initialScore;
        predecessorMap_[successorArc].predecessorBackpointer =
            wordHyp->backpointer;
        if (predecessorMap_[successorArc].arcHypIndex == invalidIndex)
          activeArcs_.push_back(successorArc);
      }
    }
  }
}

// Set_ArcLinks
void WordConditionedTreeSearch::SearchSpace::ActiveArcs::activateWithinWordArcs(
    const TreeHypothesis &treeHyp, const Score amThreshold,
    const Score bestScore) {
  for (ArcHypotheses::const_iterator arcHyp =
           arcHyps_.begin() + treeHyp.arcHypBegin;
       arcHyp != arcHyps_.begin() + treeHyp.arcHypEnd; ++arcHyp) {
    const Arc arc = arcHyp->arc;

    for (StateHypotheses::const_iterator stateHyp =
             stateHyps_.begin() + arcHyp->stateHypBegin;
         stateHyp != stateHyps_.begin() + arcHyp->stateHypEnd; ++stateHyp) {
      const HmmState state = stateHyp->state;

      // if last or next to last state then update predecessor entries
      if (state >= (HmmState)treeLexicon_.nStates(arc) - 1) {
        for (Arc successorArc = treeLexicon_.succArcBegin(arc);
             successorArc != treeLexicon_.succArcEnd(arc); ++successorArc) {
          if (amThreshold == maxScore || stateHyp->score == maxScore ||
              stateHyp->score < bestScore + amThreshold) {
            // arc was not active before?
            if (predecessorMap_[successorArc].arcHypIndex == invalidIndex) {
              predecessorMap_[successorArc].arcHypIndex = onlyStart;
              predecessorMap_[successorArc].isStarted = true;
              activeArcs_.push_back(successorArc);
            }

            if (state == treeLexicon_.nStates(arc)) {
              predecessorMap_[successorArc].predecessor = stateHyp->state;
              predecessorMap_[successorArc].predecessorScore = stateHyp->score;
              predecessorMap_[successorArc].predecessorBackpointer =
                  stateHyp->backpointer;
            } else {
              predecessorMap_[successorArc].prePredecessor = stateHyp->state;
              predecessorMap_[successorArc].prePredecessorScore =
                  stateHyp->score;
              predecessorMap_[successorArc].prePredecessorBackpointer =
                  stateHyp->backpointer;
            }
          }
        }
      }
    }
  }
}

void WordConditionedTreeSearch::SearchSpace::ActiveArcs::initialize(
    const TreeHypothesis &treeHyp, const WordHypothesis *wordHyp,
    const Score bestScore, const Score amThreshold) {
  initPredecessorMap(treeHyp);
  activateFirstGenerationArcs(wordHyp, bestScore, amThreshold);
  activateWithinWordArcs(treeHyp, amThreshold, bestScore);
}

inline HmmState
WordConditionedTreeSearch::SearchSpace::ActiveArcs::predecessor(Arc arc) const {
  require(arc >= 0 && arc < predecessorMap_.size());
  return predecessorMap_[arc].predecessor;
}

inline HmmState
WordConditionedTreeSearch::SearchSpace::ActiveArcs::prePredecessor(
    Arc arc) const {
  require(arc >= 0 && arc < predecessorMap_.size());
  return predecessorMap_[arc].prePredecessor;
}

inline Index
WordConditionedTreeSearch::SearchSpace::ActiveArcs::arcHypIndex(Arc arc) const {
  require(arc >= 0 && arc < predecessorMap_.size());
  return predecessorMap_[arc].arcHypIndex;
}

inline std::vector<Index>::const_iterator
WordConditionedTreeSearch::SearchSpace::ActiveArcs::begin() const {
  return activeArcs_.begin();
}

inline std::vector<Index>::const_iterator
WordConditionedTreeSearch::SearchSpace::ActiveArcs::end() const {
  return activeArcs_.end();
}

inline bool WordConditionedTreeSearch::SearchSpace::ActiveArcs::isStarted(
    const Arc arc) const {
  require(arc >= 0 && arc < predecessorMap_.size());
  return predecessorMap_[arc].isStarted;
}

inline Score
WordConditionedTreeSearch::SearchSpace::ActiveArcs::predecessorScore(
    Arc arc) const {
  require(arc >= 0 && arc < predecessorMap_.size());
  return predecessorMap_[arc].predecessorScore;
}

inline Score
WordConditionedTreeSearch::SearchSpace::ActiveArcs::prePredecessorScore(
    Arc arc) const {
  require(arc >= 0 && arc < predecessorMap_.size());
  return predecessorMap_[arc].prePredecessorScore;
}

inline Index
WordConditionedTreeSearch::SearchSpace::ActiveArcs::predecessorBackpointer(
    Arc arc) const {
  require(arc >= 0 && arc < predecessorMap_.size());
  return predecessorMap_[arc].predecessorBackpointer;
}

inline Index
WordConditionedTreeSearch::SearchSpace::ActiveArcs::prePredecessorBackpointer(
    Arc arc) const {
  require(arc >= 0 && arc < predecessorMap_.size());
  return predecessorMap_[arc].prePredecessorBackpointer;
}

inline bool
WordConditionedTreeSearch::SearchSpace::ActiveArcs::hasActiveWithinArcStates(
    Arc arc) const {
  require(arc >= 0 && arc < predecessorMap_.size());
  return predecessorMap_[arc].arcHypIndex != invalidIndex &&
         predecessorMap_[arc].arcHypIndex != onlyStart;
}

// -------------------------------------------------------------------------------------------------

/** maximum skip transition */
const HmmState WordConditionedTreeSearch::SearchSpace::maxSkip_ = 2;

/** time interval for garbage collection in book keeping */
const Time WordConditionedTreeSearch::SearchSpace::purgeStorageInterval_ = 50;

WordConditionedTreeSearch::SearchSpace::SearchSpace(const Lexicon &lexicon)
    : treeLexicon_(lexicon), activeTrees_(lexicon.nWords()),
      bookKeeping_(new BookKeeping) {
  activeArcs_ = new ActiveArcs(treeLexicon_, stateHypotheses_, arcHypotheses_);
}

WordConditionedTreeSearch::SearchSpace::~SearchSpace() {
  delete bookKeeping_;
  delete activeArcs_;
}

void WordConditionedTreeSearch::SearchSpace::reset() {
  require(transitionScores_.size());

  bookKeeping_->clear();
  wordHypotheses_.clear();
  stateHypotheses_.clear();
  newStateHypotheses_.clear();
  arcHypotheses_.clear();
  newArcHypotheses_.clear();
  treeHypotheses_.clear();
  newTreeHypotheses_.clear();

  bestScore_ = maxScore;
  bestState_ = inactiveState;

  activeTrees_.clear();
}

void WordConditionedTreeSearch::SearchSpace::addInitialHypothesis() {
  wordHypotheses_.push_back(WordHypothesis(treeLexicon_.silence(), 0.f, 0));
  addBookKeepingEntries(0);
}

// InsertAndPruneWordEnds
void WordConditionedTreeSearch::SearchSpace::startNewTrees() {
  bestScore_ = maxScore;
  const WordHypotheses::const_iterator wordHypBegin = wordHypotheses_.begin();

  for (WordHypotheses::const_iterator wordHyp = wordHypBegin;
       wordHyp != wordHypotheses_.end(); ++wordHyp) {
    // do not start trees conditioned on silence
    const Word word = wordHyp->word == treeLexicon_.silence()
                          ? nonSilencePredecessorWord(wordHyp->backpointer)
                          : wordHyp->word;

    // no such active tree hypothesis yet?
    if (!activeTrees_.isActive(word))
      // add active tree hypothesis without active arc hypotheses
      treeHypotheses_.push_back(
          TreeHypothesis(word, invalidIndex, invalidIndex));
    // restart tree (may still contain other arc hypotheses as well)
    activeTrees_.restartTree(word, &*wordHyp);
  }
}

void WordConditionedTreeSearch::SearchSpace::expand(
    Time time, Score acousticPruningThreshold, AcousticModelScorer &amScorer) {
  for (TreeHypotheses::const_iterator treeHyp = treeHypotheses_.begin();
       treeHyp != treeHypotheses_.end(); ++treeHyp) {
    activeArcs_->initialize(*treeHyp,
                            activeTrees_.wordHyp(treeHyp->predecessorWord),
                            bestScore_, acousticPruningThreshold);

    for (ArcIndexIterator arc = activeArcs_->begin(); arc != activeArcs_->end();
         ++arc) {
      computeTimeAlignment(*arc, amScorer, acousticPruningThreshold);
      writeBackArcHypothesis(*arc);
    }
  }
    writeBackTreeHypothesis(*treeHyp);
  }
}

void WordConditionedTreeSearch::SearchSpace::initTimeAlignment(
    Node nodes[], HmmState &maxState, const Arc arc, const HmmState nStates) {
  // initialize predecessor state
  Node *hmm = nodes + 1;

  for (int i = 0; i < nStates + 2; i++)
    nodes[i].score = maxScore;

  if (activeArcs_->predecessor(arc) != inactiveState ||
      activeArcs_->isStarted(arc)) {
    hmm[0].score = activeArcs_->predecessorScore(arc);
    hmm[0].backpointer = activeArcs_->predecessorBackpointer(arc);
  }

  if (activeArcs_->prePredecessor(arc) != inactiveState) {
    hmm[-1].score = activeArcs_->prePredecessorScore(arc);
    hmm[-1].backpointer = activeArcs_->prePredecessorBackpointer(arc);
  }

  // copy active within-arc states
  maxState = 0;
  if (activeArcs_->hasActiveWithinArcStates(arc)) {
    const ArcHypothesis &arcHyp = arcHypotheses_[activeArcs_->arcHypIndex(arc)];
    require(arcHyp.stateHypBegin >= 0 &&
            arcHyp.stateHypBegin < stateHypotheses_.size());
    require(arcHyp.stateHypEnd >= 0 &&
            arcHyp.stateHypEnd <= stateHypotheses_.size());
    for (StateHypotheses::const_iterator stateHyp =
             stateHypotheses_.begin() + arcHyp.stateHypBegin;
         stateHyp != stateHypotheses_.begin() + arcHyp.stateHypEnd;
         ++stateHyp) {
      const HmmState state = stateHyp->state;
      require(state >= 1 && state <= nStates);
      maxState = std::max(maxState, state);
      hmm[state].score = stateHyp->score;
      hmm[state].backpointer = stateHyp->backpointer;
    }
  }

  maxState = std::min((HmmState)(maxState + maxSkip_), nStates);
}

std::ostream &operator<<(std::ostream &os, std::vector<Score> &v) {
  os << '[';
  for (int i = 0; i < v.size(); ++i) {
    os << v.at(i) << ',';
  }
  os << ']';
}

// TimeAlignAndUpdate_HypArr
void WordConditionedTreeSearch::SearchSpace::computeTimeAlignment(
    const Arc arc, const AcousticModelScorer &amScorer, Score amThreshold) {

  const MixtureSequence &mixtures = treeLexicon_.mixtures(arc);//states of a phoneme
  HmmState nStates = treeLexicon_.nStates(arc);//number of states of a phoneme

  //these two scores and backpointers are for t-1
  std::vector<Score> scores(nStates + maxSkip_, maxScore);//considering the first and last state?
  std::vector<Index> backpointers(nStates + maxSkip_);

  //initialize the score an pointers
  scores.at(0) = activeArcs_->prePredecessorScore(arc);
  scores.at(1) = activeArcs_->predecessorScore(arc);
  backpointers.at(0) = activeArcs_->prePredecessorBackpointer(arc);
  backpointers.at(1) = activeArcs_->predecessorBackpointer(arc);
  if (activeArcs_->hasActiveWithinArcStates(arc)) {
    Index b = arcHypotheses_.at(activeArcs_->arcHypIndex(arc)).stateHypBegin;
    for (; b < arcHypotheses_.at(activeArcs_->arcHypIndex(arc)).stateHypEnd;b++) {
      require(stateHypotheses_.at(b).state > 0);
      scores.at(stateHypotheses_.at(b).state + 1) = stateHypotheses_.at(b).score;
      backpointers.at(stateHypotheses_.at(b).state + 1) = stateHypotheses_.at(b).backpointer;
    }
  }

  //another pair of curScores and curBackpointer for the cur phoneme(time t)
  std::vector<Score> curScores(nStates);
  std::vector<Index> back(nStates);
  for (size_t i = 0; i < nStates; i++) {
    std::vector<Score> currentScores(maxSkip_ + 1);//this currentScores is just for comparison of s-2,s-1,s
    for (size_t j = 0; j < currentScores.size(); j++) {
      bool is_silence = mixtures.at(0) == treeLexicon_.silenceMixture();
      currentScores.at(j) = scores.at(i + j) + transitionScores_.at(is_silence).at(maxSkip_ - j);
    }

    std::vector<Score>::iterator min_score = std::min_element(currentScores.begin(), currentScores.end());//select max from s,s-1,s-2
    curScores.at(i) = *min_score;//update the score at time t
    back.at(i) = backpointers.at(i + maxSkip_ - (min_score - currentScores.begin()));
  }

  for (size_t i = 0; i < nStates; i++) {
    if (curScores.at(i) < maxScore) {//check if the score is valid
      State state = i + 1;
      Score score = curScores.at(i) + amScorer(mixtures.at(i));
      Index backpointer = back.at(i);
      newStateHypotheses_.push_back(StateHypothesis(state, score, backpointer));
    }
  }
}

// Update_AllArcArr
void WordConditionedTreeSearch::SearchSpace::writeBackArcHypothesis(Arc arc) {
  // active state hypotheses for current arc?
  if (newArcHypotheses_.empty()) {
    if (!newStateHypotheses_.empty())
      newArcHypotheses_.push_back(
          ArcHypothesis(arc, 0, newStateHypotheses_.size()));
  } else if (newArcHypotheses_.back().stateHypEnd <
             newStateHypotheses_.size()) {
    newArcHypotheses_.push_back(ArcHypothesis(
        arc, newArcHypotheses_.back().stateHypEnd, newStateHypotheses_.size()));
  }
}

// Update_TreeArr
void WordConditionedTreeSearch::SearchSpace::writeBackTreeHypothesis(
    TreeHypothesis treeHyp) {
  // processing of tree hypothesis complete (may be reactivated after pruning)
  activeTrees_.setInactive(treeHyp.predecessorWord);

  // active arc hypotheses for current tree?
  if (newTreeHypotheses_.empty()) {
    if (!newArcHypotheses_.empty())
      newTreeHypotheses_.push_back(
          TreeHypothesis(treeHyp.predecessorWord, 0, newArcHypotheses_.size()));
  } else if (newTreeHypotheses_.back().arcHypEnd < newArcHypotheses_.size()) {
    newTreeHypotheses_.push_back(TreeHypothesis(
        treeHyp.predecessorWord, newTreeHypotheses_.back().arcHypEnd,
        newArcHypotheses_.size()));
  }
}

// shiftAndPruneAndReportWordEnds
void WordConditionedTreeSearch::SearchSpace::pruneStatesAndFindWordEnds(Score acousticPruningThreshold) {
  bestScore_ = maxScore;
  for (StateHypotheses::iterator it = newStateHypotheses_.begin(); it < newStateHypotheses_.end(); it++)//find the min score from newStateHypotheses
    if (it->score < bestScore_) bestScore_ = it->score;

  //clear all the original Hypotheses vectors
  treeHypotheses_.clear();
  arcHypotheses_.clear();
  stateHypotheses_.clear();

  //Hierachical structure: Tree->Arc->State
  for (size_t i = 0; i < newTreeHypotheses_.size(); i++) {
    TreeHypothesis& treeHyp = newTreeHypotheses_.at(i);
    size_t added_arcs = 0;
    for (size_t j = treeHyp.arcHypEnd; j < treeHyp.arcHypEnd; j++) {
      ArcHypothesis& arcHyp = newArcHypotheses_.at(j);
      size_t added_states = 0;
      for (size_t k = arcHyp.stateHypBegin; k < arcHyp.stateHypEnd; k++) {
        StateHypothesis& stateHyp = newStateHypotheses_.at(k);
        Score threshold = bestScore_ + acousticPruningThreshold;
        if (stateHyp.score <= threshold) {
          stateHypotheses_.push_back(stateHyp);
          added_states++;
        }
      }
      if (added_states > 0) {//some states of this arc are valid
        arcHypotheses_.push_back(ArcHypothesis(arcHyp.arc, stateHypotheses_.size() - added_states, stateHypotheses_.size()));
        added_arcs++;
      }
    }
    if (added_arcs > 0) {
      treeHypotheses_.push_back(TreeHypothesis(treeHyp.predecessorWord, arcHypotheses_.size() - added_arcs, arcHypotheses_.size()));
    }
  }

  wordHypotheses_.clear();

  //deal with the ending word
  for (size_t i = 0; i < arcHypotheses_.size(); i++) {
    ArcHypothesis& ah = arcHypotheses_.at(i);
    Word endingWord = treeLexicon_.endingWord(ah.arc);
    if (endingWord == invalidWord) continue;

    StateHypothesis sh = stateHypotheses_.at(ah.stateHypEnd - 1);
    if (sh.state < treeLexicon_.nStates(ah.arc)) continue;//still not reach the last state of the phoneme
    Score exitPenalty = treeLexicon_.silence() == endingWord
        ? transitionScores_.at(1).at(TransitionModelScorer::exitPenaltyIndex)
        : transitionScores_.at(0).at(TransitionModelScorer::exitPenaltyIndex);//check the last word if it's silence
    std::cerr << "Word ending " << endingWord << " " << treeLexicon_.silence() << " "  << sh.backpointer << " " << sh.score << " " << exitPenalty << std::endl;
    WordHypothesis wh(endingWord, sh.score + exitPenalty, sh.backpointer);
    wordHypotheses_.push_back(wh);
  }
}

// BigramRecombination
void WordConditionedTreeSearch::SearchSpace::bigramRecombination(const LanguageModelScorer &lmScore) {
  const WordHypotheses::iterator wordHypBegin = wordHypotheses_.begin();

  for (WordHypotheses::iterator wordHyp = wordHypBegin; wordHyp != wordHypotheses_.end(); ++wordHyp) {
    if (wordHyp->word == treeLexicon_.silence()) continue;//check every word which is not silence
    const Word word = nonSilencePredecessorWord(wordHyp->backpointer);//the predecessorWord could be silence, but successWord must not be silence
    wordHyp->score += lmScore(word, wordHyp->word);
  }
}

void WordConditionedTreeSearch::SearchSpace::addBookKeepingEntries(Time time) {
  if (time % purgeStorageInterval_ == 0)
    bookKeeping_->tagActiveEntries(time, stateHypotheses_.begin(),
                                   stateHypotheses_.end());

  for (WordHypotheses::iterator wordHyp = wordHypotheses_.begin();
       wordHyp != wordHypotheses_.end(); ++wordHyp) {
    Index newBackpointer = bookKeeping_->addEntry(wordHyp->word, wordHyp->score,
                                                  wordHyp->backpointer, time);
    wordHyp->backpointer = newBackpointer;

    if (time == 0) {
      // self loop
      bookKeeping_->entry(newBackpointer).backpointer = newBackpointer;
    }

    // avoid chains of silence
    if (wordHyp->word == treeLexicon_.silence()) {
      BookKeeping::Entry &curEntry = bookKeeping_->entry(newBackpointer);
      if (bookKeeping_->entry(curEntry.backpointer).word ==
          treeLexicon_.silence())
        curEntry.backpointer =
            bookKeeping_->entry(curEntry.backpointer).backpointer;
    }
  }
}

void WordConditionedTreeSearch::SearchSpace::traceback(
    Traceback &result) const {
  result.clear();
  if (wordHypotheses_.empty())
    return;
  WordHypotheses::const_iterator best =
      std::min_element(wordHypotheses_.begin(), wordHypotheses_.end());
  Index backpointer = best->backpointer;
  while (bookKeeping_->entry(backpointer).time > 0) {
    result.push_back(bookKeeping_->entry(backpointer));
    backpointer = bookKeeping_->entry(backpointer).backpointer;
  }
  std::reverse(result.begin(), result.end());
}

void WordConditionedTreeSearch::SearchSpace::setTransitionScores(
    const TransitionModelScorer &transitionScore) {
  transitionScores_.resize(2);
  require(TransitionModelScorer::exitPenaltyIndex == 1 + maxSkip_);
  for (int isSilence = 0; isSilence <= 1; ++isSilence) {
    transitionScores_[isSilence].resize(maxSkip_ + 2);
    for (int s = 0; s <= maxSkip_; ++s)
      transitionScores_[isSilence][s] = transitionScore(isSilence, s);
    transitionScores_[isSilence][maxSkip_ + 1] =
        transitionScore(isSilence, TransitionModelScorer::exitPenaltyIndex);
  }
}

inline Word WordConditionedTreeSearch::SearchSpace::nonSilencePredecessorWord(
    Index backpointer) {
  const Word predecessorWord = bookKeeping_->entry(backpointer).word;

  if (predecessorWord != treeLexicon_.silence())
    return predecessorWord;

  return bookKeeping_->entry(bookKeeping_->entry(backpointer).backpointer).word;
}

inline bool
WordConditionedTreeSearch::SearchSpace::isSilence(const Mixture mixture) const {
  return mixture == treeLexicon_.silenceMixture();
}

// -------------------------------------------------------------------------------------------------

const Core::ParameterFloat
    WordConditionedTreeSearch::paramAcousticPruningThreshold_(
        "acoustic-pruning", "acoustic pruning threshold", maxScore, 0.);

WordConditionedTreeSearch::WordConditionedTreeSearch(
    const Core::Configuration &config)
    : Core::Component(config), SearchInterface(config),
      amScorer_(new AcousticModelScorer(this)),
      lmScorer_(new LanguageModelScorer(this)) {
  acousticPruningThreshold_ = paramAcousticPruningThreshold_(config);
  if (paramAcousticPruningThreshold_(config) > maxScore)
    acousticPruningThreshold_ = maxScore;
}

WordConditionedTreeSearch::~WordConditionedTreeSearch() {
  delete searchSpace_;
  delete amScorer_;
  delete lmScorer_;
}

bool WordConditionedTreeSearch::setModelCombination(
    const Speech::ModelCombination &modelCombination) {
  SearchInterface::setModelCombination(modelCombination);
  searchSpace_ = new SearchSpace(*lexicon_);
  searchSpace_->setTransitionScores(TransitionModelScorer(this));
  return true;
}

void WordConditionedTreeSearch::initialize() {
  searchSpace_->reset();
  searchSpace_->addInitialHypothesis();
}

// WordDepTreeSearch
void WordConditionedTreeSearch::processFrame(Time time) {
  searchSpace_->startNewTrees();
  searchSpace_->expand(time, acousticPruningThreshold_, *amScorer_);
  searchSpace_->pruneStatesAndFindWordEnds(acousticPruningThreshold_);
  searchSpace_->bigramRecombination(*lmScorer_);
  searchSpace_->addBookKeepingEntries(time);
}

void WordConditionedTreeSearch::getResult(Traceback &result) const {
  searchSpace_->traceback(result);
}
