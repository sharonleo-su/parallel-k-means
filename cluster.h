#include <cstdint>
#include <vector>
#include <cmath>

// The type used to represent points on a 2D plane
struct Point {
    float x;
    float y;
};

// A cluster is just a vector of points
using Cluster = std::vector<Point>;

// A cluster list is just a vector of clusters
using ClusterList = std::vector<Cluster>;


// desc: Displays the provided cluster list, using lower_bounds and upper_bounds to determing the
//       minimum/maximum x and y coordinates that will be viewable in the rendered image. The size of the
//       rendered image is scaled by `scale` and categories are color-coded when `color` is true
//  pre: The supplied cluster list must have 26 or fewer elements, since it uses letters to distinguish
//       different clusters in the display
// post:
void display_clusters(ClusterList list, Point lower_bounds, Point upper_bounds, float scale, bool color);

// desc: Generates a cluster centered at `center` with `count` points and an averge spread of `spread`
//  pre: None
// post: None, aside from description
Cluster generate_cluster(Point center, size_t count, float spread);

// desc: Generates a cluster list with clusters contained within the x/y minimum/maximum bounds described by
//       `lower_bounds` and `upper_bounds`, generating `cluster_count` clusters each with at most
//       `max_point_count` points
//  pre: None
// post: None
ClusterList generate_cluster_list(Point lower_bounds, Point upper_bounds, size_t cluster_count, size_t point_count);

// desc: Combines all points contained by a cluster list into a single cluster.
//  pre: None
// post: None
Cluster collapse_cluster_list(ClusterList list);

