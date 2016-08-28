#include <vector>
#include <map>
#include <set>
#include <opencv2/core/core.hpp>

namespace Popup
{
  struct FoldLine //fold line structure
  {
  FoldLine(const std::pair<int, int> _original_patch_pair, const int _desirable_center, const double _score) : is_original_fold_line(_original_patch_pair.first != _original_patch_pair.second), original_patch_pair(_original_patch_pair), desirable_center(_desirable_center), score(_score), optimized_activity(-1), optimized_convexity(-1), optimized_position(-1) {};
  FoldLine(const std::pair<int, int> _original_patch_pair, const std::vector<int> &_positions) : is_original_fold_line(_original_patch_pair.first != _original_patch_pair.second), original_patch_pair(_original_patch_pair), positions(_positions), optimized_activity(-1), optimized_convexity(-1), optimized_position(-1) {};
    bool is_original_fold_line; //whether this fold line is between two original patches or not
    //int original_patch_index; //the index of the fold line's original patch
    std::pair<int, int> original_patch_pair; //(left_original_patch, right_original_patch) pair. For new fold lines, left_original_patch == right_original_patch.
    int desirable_center; //the most desirable position for this fold line
    double score; //the score of the fold line at the most desirable position
    std::vector<int> positions; //the x range of this fold line
    std::set<int> line_segment_indices; //line segments for this fold line (used only for original fold lines in case two original fold lines have overlap)
    //std::map<int, double> x_score_map; //scores for different x positions
    int optimized_activity;
    int optimized_convexity;
    int optimized_position; //optimized x position for the fold line (-1 for inactive fold lines)
    std::vector<int> optimized_pixels; //optimized pixel positions for the fold line
  };

  class PopupGraph
  {
  public:
    
    PopupGraph(const std::vector<int> &patch_index_mask, const int IMAGE_WIDTH, const int IMAGE_HEIGHT, const int FOLD_LINE_WINDOW_WIDTH, const int FOLD_LINE_WINDOW_HEIGHT, const int MIDDLE_FOLD_LINE_X, const std::set<int> &island_patches, const bool ENFORCE_SYMMETRY, const bool BUILD_COMPLETE_POPUP_GRAPH, const int MIN_FOLD_LINE_GAP);

    int getImageWidth() const { return IMAGE_WIDTH_; };
    int getImageHeight() const { return IMAGE_HEIGHT_; };
    int getNumFoldLines() const { return fold_lines_.size(); };
    int getNumOriginalFoldLines() const { return NUM_ORIGINAL_FOLD_LINES_; };
    int getMiddleFoldLineIndex() const { return MIDDLE_FOLD_LINE_INDEX_; };
    int getMiddleFoldLineX() const { return MIDDLE_FOLD_LINE_X_; };
    int getNumOriginalPatches() const { return NUM_ORIGINAL_PATCHES_; };
    int getOriginalBackgroundPatchIndex() const { return ORIGINAL_BACKGROUND_PATCH_INDEX_; };
    std::pair<int, int> getBorderFoldLineIndices() const { return std::make_pair(LEFT_BORDER_FOLD_LINE_INDEX_, RIGHT_BORDER_FOLD_LINE_INDEX_); };
    std::vector<std::pair<int, int> > getFoldLinePairs() const { return fold_line_pairs_; };
    std::vector<std::pair<int, int> > getFoldLinePairsWithoutPassing() const { return fold_line_pairs_without_passing_; };
    std::vector<std::pair<int, int> > getFoldLinePairsPassed() const;
    std::map<int, std::map<int, std::vector<int> > > getFoldLineLeftPaths() const { return fold_line_left_paths_; };
    std::map<int, std::map<int, std::vector<int> > > getFoldLineRightPaths() const { return fold_line_right_paths_; };
    std::pair<int, int> getFoldLineXRange(const int fold_line_index) const;
    double getFoldLineScore(const int fold_line_index) const { return fold_lines_[fold_line_index].score; };
    std::map<int, std::map<int, std::set<int> > > getPatchNeighborFoldLines(const char direction, const std::map<int, std::set<int> > &patch_child_patches = std::map<int, std::set<int> >()) const;
    std::map<int, std::set<int> > getPatchFoldLines() const;
    bool getEnforceSymmetryFlag() const { return ENFORCE_SYMMETRY_; };
    std::vector<FoldLine> getFoldLines() const { return fold_lines_; };
    std::vector<int> getPatchIndexMask() const { return patch_index_mask_; };
    
    cv::Mat drawOriginalPopupGraph();
    cv::Mat drawPopupGraph();
    cv::Mat drawOptimizedPopupGraph();

    std::vector<int> getBackgroundLeftFoldLines() const;
    std::vector<int> getBackgroundRightFoldLines() const;

    int getMinFoldLineGap() const { return MIN_FOLD_LINE_GAP_; };

    void checkFoldLinePairs() const;
    void checkFoldLinePaths() const;
    void checkFoldLineInfo() const;

    void setOptimizedFoldLineInfo(const std::vector<int> &optimized_fold_line_activities, const std::vector<int> &optimized_fold_line_convexities, const std::vector<int> &optimized_fold_line_positions);
    void setOptimizedFoldLineInfo(const int fold_line_index, const int optimized_activity, const int optimized_convexity = -1, const int optimized_position = -1);
    void getOptimizedFoldLineInfo(std::vector<int> &optimized_fold_line_activities, std::vector<int> &optimized_fold_line_convexities, std::vector<int> &optimized_fold_line_positions);
    void getDesirableFoldLinePositions(std::vector<int> &desirable_fold_line_positions) const;
    
    std::map<int, std::set<int> >  getIslandPatchInfo() const;
    std::map<int, std::set<int> > getPatchChildPatches() const { return patch_child_patches_; };
    
    std::set<int> getSymmetryFoldLines();
    std::vector<std::pair<int, int> > getSymmetricFoldLinePairs() const;
    std::vector<int> getNewFoldLines() const;

    void addOptimizedInfo(const PopupGraph &optimized_popup_graph);

    void writeRenderingInfo();
    
    
  private:
    const int IMAGE_WIDTH_;
    const int IMAGE_HEIGHT_;
    const int FOLD_LINE_WINDOW_WIDTH_;
    const int FOLD_LINE_WINDOW_HEIGHT_;
    const int MIDDLE_FOLD_LINE_X_;
    const bool ENFORCE_SYMMETRY_;
    const bool MIN_FOLD_LINE_GAP_;
    
    int NUM_ORIGINAL_FOLD_LINES_;
    int NUM_ORIGINAL_PATCHES_;
    int NUM_LINE_SEGMENTS_;

    int ORIGINAL_BACKGROUND_PATCH_INDEX_;
    int MIDDLE_FOLD_LINE_INDEX_;
    int LEFT_BORDER_FOLD_LINE_INDEX_;
    int RIGHT_BORDER_FOLD_LINE_INDEX_;
    int BUILD_COMPLETE_POPUP_GRAPH_;
    

    std::vector<std::pair<int, int> > fold_line_pairs_;
    std::vector<std::pair<int, int> > fold_line_pairs_without_passing_;
    std::map<int, std::map<int, std::vector<int> > > fold_line_left_paths_;
    std::map<int, std::map<int, std::vector<int> > > fold_line_right_paths_;

    std::vector<FoldLine> fold_lines_;
    std::vector<int> line_segment_fold_line_indices_;
    std::vector<int> pixel_line_segment_indices_;
    std::vector<int> patch_index_mask_;
    std::map<int, std::set<int> > patch_child_patches_;
    std::map<int, int> symmetric_patch_map_;
    std::set<int> symmetry_fold_lines_;
    

    void countNumOriginalPatches();
    void findOriginalBackgroundPatchIndex();
    void enforceSymmetry();
    void calcLineSegmentInfo();
    void findOriginalFoldLines();
    void findAllFoldLines();
    void findFoldLinePairs();
    void findFoldLinePaths();
    void findPatchChildPatches(const std::set<int> &island_patches);
    void buildSubGraphes();
    
      //std::map<int, std::map<int, std::set<int> > > original_patch_fold_lines;
    //std::set<int> background_patches;
    //std::map<int, std::map<int, double> > fold_line_x_score_map;
  };
}
