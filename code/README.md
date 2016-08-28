# Popup Design

### Requirements

* OpenCV
* Gurobi
* gflags

### Instruction

**To compile the program:**

0. mkdir build
1. cd build
2. cmake ..
3. make


**Options:**

DEFINE_string(patch_index_mask_image_path, "Examples/bee/patch_index_mask_image.png", "The patch index mask image path. Patch index mask image is a color image with different colors indicating different patches. (A patch with gray color is treated as an island patch (such as eye patches).");

DEFINE_bool(enforce_symmetry, false, "Whether symmetry is enforced or not.");

DEFINE_int32(middle_fold_line_x_offset, 0, "Denote where the fold line should be put (image_width /  2 + offset).");

DEFINE_int32(fold_line_window_width, 10, "The preferable window width for a fold line.");

DEFINE_int32(fold_line_window_height, 10, "The preferable window height for a fold line.");

DEFINE_int32(min_fold_line_gap, 5, "The minimum distance between two neighboring fold lines.");

DEFINE_bool(write_intermediate_results, true, "Whether intermediate results are saved or not.");

DEFINE_string(output_folder, "Examples", "The output folder.");

DEFINE_string(output_prefix, "", "The output prefix (to distinguish different results.");


**Examples:**

./PopupDesign -patch_index_mask_image_path="Examples/bear/patch_index_mask_image.png" -enforce_symmetry=true -middle_fold_line_x_offset=-10 -output_folder="Results" -output_prefix="bear_" -min_fold_line_gap=1 -fold_line_window_width=15 -fold_line_window_height=15

./PopupDesign -patch_index_mask_image_path="Examples/goat/patch_index_mask_image.png" -enforce_symmetry=false -output_folder="Results" -output_prefix="goat_"

./PopupDesign -patch_index_mask_image_path="Examples/cow/patch_index_mask_image.png" -enforce_symmetry=false -output_folder="Results" -output_prefix="cow_"

./PopupDesign -patch_index_mask_image_path="Examples/bee/patch_index_mask_image.png" -enforce_symmetry=false -output_folder="Results" -output_prefix="bee_"
