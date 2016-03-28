#include <vector>
#include <opencv2/core/core.hpp>

namespace Popup{
  std::vector<int> zoomMask(const std::vector<int> &mask, const int IMAGE_WIDTH, const int IMAGE_HEIGHT, const int NEW_IMAGE_WIDTH, const int NEW_IMAGE_HEIGHT);
  std::vector<int> findNeighbors(const int pixel, const int IMAGE_WIDTH, const int IMAGE_HEIGHT);
  cv::Mat drawIndexMaskImage(const std::vector<int> &index_mask, const int IMAGE_WIDTH, const int IMAGE_HEIGHT);
}
