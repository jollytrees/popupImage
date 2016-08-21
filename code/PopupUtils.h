#include <vector>
#include <opencv2/core/core.hpp>

namespace Popup{
  std::vector<int> zoomMask(const std::vector<int> &mask, const int IMAGE_WIDTH, const int IMAGE_HEIGHT, const int NEW_IMAGE_WIDTH, const int NEW_IMAGE_HEIGHT);
  std::vector<int> findNeighbors(const int pixel, const int IMAGE_WIDTH, const int IMAGE_HEIGHT);
  cv::Mat drawIndexMaskImage(const std::vector<int> &index_mask, const int IMAGE_WIDTH, const int IMAGE_HEIGHT);
  void grabCut(const cv::Mat &image, const std::string &result_filename);
  //void deformPatches(const int IMAGE_WIDTH, const int IMAGE_HEIGHT, const int NUM_PATCHES, const std::vector<int> &patch_index_mask, const std::vector<int> &enforced_patch_index_mask, std::vector<int> &deformed_patch_index_mask);
}
