#include "cluster.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mpi.h>

std::string const USAGE = "Program requires exactly two arguments, both positive integers.\n";

void              bad_usage() {
    std::cerr << USAGE;
    std::exit(1);
}

void get_args(int argc, char *argv[], int &cluster_count, int &point_count) {
    if (argc <= 2) {
        bad_usage();
    } else if (argc >= 4) {
        bad_usage();
    }

    cluster_count = 0;
    point_count   = 0;

    cluster_count = atoi(argv[1]);
    point_count   = atoi(argv[2]);

    if ((cluster_count <= 0) || (point_count <= 0)) {
        bad_usage();
    }
}

float distance(Point const &a, Point const &b) {
    float x_diff = a.x - b.x;
    float y_diff = a.y - b.y;
    return sqrt(x_diff * x_diff + y_diff * y_diff);
}

bool recenter(ClusterList cluster_list, std::vector<Point> &centers) {
    size_t cluster_count = cluster_list.size();
    bool   converged     = true;
    for (size_t i = 0; i < cluster_count; i++) {
        if (cluster_list[i].empty()) {
            continue;
        }
        Point avg{0, 0};
        for (Point &point : cluster_list[i]) {
            avg.x += point.x;
            avg.y += point.y;
        }
        size_t size = cluster_list[i].size();
        avg.x /= size;
        avg.y /= size;
        bool x_matches = (centers[i].x == avg.x);
        bool y_matches = (centers[i].y == avg.y);
        if (!x_matches || !y_matches) {
            converged = false;
        }
        centers[i] = avg;
    }
    return converged;
}

ClusterList reassign(ClusterList cluster_list, std::vector<Point> centers) {
    ClusterList result;
    size_t      cluster_count = cluster_list.size();
    result.resize(cluster_count);
    for (Cluster &cluster : cluster_list) {
        for (Point &point : cluster) {
            size_t best          = 0;
            float  best_distance = std::numeric_limits<float>::infinity();
            for (size_t i = 0; i < cluster_count; i++) {
                float dist = distance(point, centers[i]);
                if (dist < best_distance) {
                    best          = i;
                    best_distance = dist;
                }
            }
            result[best].push_back(point);
        }
    }
    return result;
}

ClusterList k_means(std::vector<Point> points, size_t cluster_count) {
    std::vector<Point> centers;
    centers.resize(cluster_count);

    for (Point &point : centers) {
        point = {(float)(rand() % 1000) / 1000, (float)(rand() % 1000) / 1000};
    }

    ClusterList clusters;
    clusters.resize(cluster_count);
    clusters[0]            = points;
    bool   converged       = false;
    size_t iteration_limit = 50;
    size_t iteration_count = 0;
    while ((!converged) && (iteration_count < iteration_limit)) {
        clusters  = reassign(clusters, centers);
        converged = recenter(clusters, centers);
        iteration_count++;
    }
    return clusters;
}

ClusterList parallel_k_means(std::vector<Point> points, size_t cluster_count) {
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Calculate the number of points each process will handle
    int local_point_count = points.size() / size;
    int remaining_points  = points.size() % size;

    // Calculate the starting index for each process
    size_t start_index = rank * local_point_count + std::min(rank, remaining_points);
    size_t end_index   = start_index + local_point_count + (rank < remaining_points ? 1 : 0);

    // Distribute points to each process
    std::vector<Point> local_points(points.begin() + start_index, points.begin() + end_index);

    // Run local k_means
    ClusterList local_clusters = k_means(local_points, cluster_count);

    // Gather local cluster centers and point counts using MPI_Reduce
    std::vector<Point>  local_centers(cluster_count, {0, 0});
    std::vector<size_t> local_point_counts(cluster_count, 0);

    for (size_t i = 0; i < local_clusters.size(); ++i) {
        for (Point const &point : local_clusters[i]) {
            local_centers[i].x += point.x;
            local_centers[i].y += point.y;
            local_point_counts[i]++;
        }
    }

    std::vector<Point>  global_centers(cluster_count, {0, 0});
    std::vector<size_t> global_point_counts(cluster_count, 0);

    // Reduce local cluster centers and point counts using MPI_Reduce
    MPI_Reduce(local_centers.data(), global_centers.data(), cluster_count * 2, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(
        local_point_counts.data(),
        global_point_counts.data(),
        cluster_count,
        MPI_UNSIGNED_LONG,
        MPI_SUM,
        0,
        MPI_COMM_WORLD
    );

    // Calculate global cluster centers as the average on the root process
    if (rank == 0) {
        for (size_t i = 0; i < cluster_count; ++i) {
            if (global_point_counts[i] > 0) {
                global_centers[i].x /= global_point_counts[i];
                global_centers[i].y /= global_point_counts[i];
            }
        }
    }

    // Print debugging information
    // std::cout << "Rank " << rank << " local_centers: ";
    // for (const auto& center : local_centers) {
    //     std::cout << "(" << center.x << ", " << center.y << ") ";
    // }
    // std::cout << "local_point_counts: ";
    // for (const auto& count : local_point_counts) {
    //     std::cout << count << " ";
    // }
    // std::cout << std::endl;

    // Broadcast global cluster centers to all processes
    MPI_Bcast(global_centers.data(), cluster_count * 2, MPI_FLOAT, 0, MPI_COMM_WORLD);

    // Reassign points based on global cluster centers
    local_clusters = reassign(local_clusters, global_centers);

    return local_clusters;
}

int main(int argc, char *argv[]) {
    int cluster_count, point_count;
    get_args(argc, argv, cluster_count, point_count);

    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    srand(time(nullptr));
    Point       lower_bounds = {0, 0};
    Point       upper_bounds = {1, 1};

    ClusterList list         = generate_cluster_list(lower_bounds, upper_bounds, cluster_count, point_count);

    // if (rank == 0) {
    //     display_clusters(list, {0, 0}, {1, 1}, 40, true);
    // }

    std::vector<Point> collapsed = collapse_cluster_list(list);

    // Measure the runtime of parallel_k_means
    MPI_Barrier(MPI_COMM_WORLD); // Synchronize processes
    double start_time = MPI_Wtime();

    // Attempt to find the clusters in parallel
    ClusterList kmeans = parallel_k_means(collapsed, cluster_count);

    MPI_Barrier(MPI_COMM_WORLD); // Synchronize processes
    double end_time = MPI_Wtime();

    if (rank == 0) {
        // Display the clusters, as found by the parallel_k_means algorithm
        // display_clusters(kmeans, {0, 0}, {1, 1}, 40, true);

        // Print the runtime in milliseconds
        std::cout << std::fixed << std::setprecision(4) << (end_time - start_time) << "\n";
    }

    MPI_Finalize();

    return 0;
}
