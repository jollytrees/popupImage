#include <vector>
#include <map>
#include <set>
#include <opencv2/core/core.hpp>

namespace Popup
{
  struct FoldLine //fold line structure
  {
  FoldLine(const std::pair<int, int> _original_patch_pair, const int _desirable_position, const double _score) : is_original_fold_line(true), original_patch_pair(_original_patch_pair), desirable_position(_desirable_position), score(_score) {};
  FoldLine(const std::pair<int, int> _original_patch_pair, const std::vector<int> &_positions) : is_original_fold_line(false), original_patch_pair(_original_patch_pair), positions(_positions) {};
    bool is_original_fold_line; //whether this fold line is between two original patches or not
    //int original_patch_index; //the index of the fold line's original patch
    std::pair<int, int> original_patch_pair; //(left_original_patch, right_original_patch) pair. For new fold lines, left_original_patch == right_original_patch.
    int desirable_position; //the most desirable position for this fold line
    double score; //the score of the fold line at the most desirable position
    std::vector<int> positions; //the x range of this fold line
    //std::map<int, double> x_score_map; //scores for different x positions
    int optimized_position;
  };

  class PopupGraph
  {
  public:
    
    PopupGraph(const std::vector<int> &patch_index_mask, const int IMAGE_WIDTH, const int IMAGE_HEIGHT, const int FOLD_LINE_WINDOW_WIDTH, const int FOLD_LINE_WINDOW_HEIGHT, const int MIDDLE_FOLD_LINE_X = -1);

    int getImageWidth() const { return IMAGE_WIDTH_; };
    int getImageHeight() const { return IMAGE_HEIGHT_; };
    int getNumFoldLines() const { return fold_lines_.size(); };
    int getNumOriginalFoldLines() const { return NUM_ORIGINAL_FOLD_LINES_; };
    int getMiddleFoldLineIndex() const { return MIDDLE_FOLD_LINE_INDEX_; };
    int getNumOriginalPatches() const { return NUM_ORIGINAL_PATCHES_; };
    int getOriginalBackgroundPatchIndex() const { return ORIGINAL_BACKGROUND_PATCH_INDEX_; };
    std::pair<int, int> getBorderFoldLineIndices() const { return std::make_pair(LEFT_BORDER_FOLD_LINE_INDEX_, RIGHT_BORDER_FOLD_LINE_INDEX_); };
    std::vector<std::pair<int, int> > getFoldLinePairs() const { return fold_line_pairs_; };
    std::map<int, std::map<int, std::vector<int> > > getFoldLineLeftPaths() const { return fold_line_left_paths_; };
    std::map<int, std::map<int, std::vector<int> > > getFoldLineRightPaths() const { return fold_line_right_paths_; };
    std::pair<int, int> getFoldLineXRange(const int fold_line_index) const;
    std::map<int, std::map<int, std::set<int> > > getPatchNeighborFoldLines() const;
    
    cv::Mat drawOriginalPopupGraph();
    cv::Mat drawPopupGraph();

    std::vector<int> getBackgroundLeftFoldLines() const;
    std::vector<int> getBackgroundRightFoldLines() const;

    int getMinFoldLineGap() const { return FOLD_LINE_WINDOW_WIDTH_ / 2; };
    
    
  private:
    const int IMAGE_WIDTH_;
    const int IMAGE_HEIGHT_;
    const int FOLD_LINE_WINDOW_WIDTH_;
    const int FOLD_LINE_WINDOW_HEIGHT_;
    const int MIDDLE_FOLD_LINE_X_;
    
    int NUM_ORIGINAL_FOLD_LINES_;
    int NUM_ORIGINAL_PATCHES_;
    int NUM_LINE_SEGMENTS_;

    int ORIGINAL_BACKGROUND_PATCH_INDEX_;
    int MIDDLE_FOLD_LINE_INDEX_;
    int LEFT_BORDER_FOLD_LINE_INDEX_;
    int RIGHT_BORDER_FOLD_LINE_INDEX_;
    

    std::vector<std::pair<int, int> > fold_line_pairs_;
    std::map<int, std::map<int, std::vector<int> > > fold_line_left_paths_;
    std::map<int, std::map<int, std::vector<int> > > fold_line_right_paths_;

    std::vector<FoldLine> fold_lines_;
    std::vector<int> line_segment_fold_line_indices_;
    std::vector<int> pixel_line_segment_indices_;
    std::vector<int> patch_index_mask_;
    
    
    void calcLineSegmentInfo();
    void findOriginalFoldLines();
    void findAllFoldLines();
    void findFoldLinePairs();
    void findFoldLinePaths();
    
    void checkFoldLinePairs();
    void checkFoldLinePaths();
    void checkFoldLineInfo();
    
      //std::map<int, std::map<int, std::set<int> > > original_patch_fold_lines;
    //std::set<int> background_patches;
    //std::map<int, std::map<int, double> > fold_line_x_score_map;
  };
}
