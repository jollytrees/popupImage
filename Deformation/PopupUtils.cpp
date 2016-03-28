#include "PopupUtils.h"

#include <cmath>
#include <map>

using namespace std;
using namespace cv;

namespace Popup
{
  vector<int> zoomMask(const vector<int> &mask, const int IMAGE_WIDTH, const int IMAGE_HEIGHT, const int NEW_IMAGE_WIDTH, const int NEW_IMAGE_HEIGHT)
  {
    vector<int> zoomed_mask(NEW_IMAGE_WIDTH * NEW_IMAGE_HEIGHT);
    for (int zoomed_pixel = 0; zoomed_pixel < NEW_IMAGE_WIDTH * NEW_IMAGE_HEIGHT; zoomed_pixel++) {
      int zoomed_x = zoomed_pixel % NEW_IMAGE_WIDTH;
      int zoomed_y = zoomed_pixel / NEW_IMAGE_WIDTH;
      int x = round(1.0 * zoomed_x / NEW_IMAGE_WIDTH * IMAGE_WIDTH);
      x = min(x, IMAGE_WIDTH - 1);
      int y = round(1.0 * zoomed_y / NEW_IMAGE_HEIGHT * IMAGE_HEIGHT);
      y = min(y, IMAGE_HEIGHT - 1);
      zoomed_mask[zoomed_pixel] = mask[y * IMAGE_WIDTH + x];
    }
    return zoomed_mask;
  }
  
  vector<int> findNeighbors(const int pixel, const int IMAGE_WIDTH, const int IMAGE_HEIGHT)
  {
    int x = pixel % IMAGE_WIDTH;
    int y = pixel / IMAGE_WIDTH;
    vector<int> neighbor_pixels;
    if (x > 0)
      neighbor_pixels.push_back(pixel - 1);
    if (x < IMAGE_WIDTH - 1)
      neighbor_pixels.push_back(pixel + 1);
    if (y > 0)
      neighbor_pixels.push_back(pixel - IMAGE_WIDTH);
    if (y < IMAGE_HEIGHT - 1)
      neighbor_pixels.push_back(pixel + IMAGE_WIDTH);
    if (x > 0 && y > 0)
      neighbor_pixels.push_back(pixel - IMAGE_WIDTH - 1);
    if (x < IMAGE_WIDTH - 1 && y > 0)
      neighbor_pixels.push_back(pixel - IMAGE_WIDTH + 1);
    if (x > 0 && y < IMAGE_HEIGHT - 1)
      neighbor_pixels.push_back(pixel + IMAGE_WIDTH - 1);
    if (x < IMAGE_WIDTH - 1 && y < IMAGE_HEIGHT - 1)
      neighbor_pixels.push_back(pixel + IMAGE_WIDTH + 1);
    return neighbor_pixels;
  }

  Mat drawIndexMaskImage(const vector<int> &index_mask, const int IMAGE_WIDTH, const int IMAGE_HEIGHT)
  {
    Mat index_mask_image = Mat::zeros(IMAGE_HEIGHT, IMAGE_WIDTH, CV_8UC3);
    map<int, Vec3b> color_table;
    for (int pixel = 0; pixel < IMAGE_WIDTH * IMAGE_HEIGHT; pixel++) {
      int index = index_mask[pixel];
      if (index < 0)
	continue;
      if (color_table.count(index) == 0)
	color_table[index] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
      index_mask_image.at<Vec3b>(pixel / IMAGE_WIDTH, pixel % IMAGE_WIDTH) = color_table[index];
    }
    return index_mask_image;
  }
}
