
#include <ros/ros.h>
#include <ros/console.h>
#include <geometry_msgs/PoseStamped.h>
#include <nav_msgs/Odometry.h>
#include <tf/tf.h>
#include <cmath>
#include <initializer_list>

#define ROSCONSOLE_MIN_SEVERITY ROSCONSOLE_SEVERITY_DEBUG //ROSCONSOLE_SEVERITY_INFO, ROSCONSOLE_SEVERITY_DEBUG
using geometry_msgs::PoseStamped;
using std::vector;
using std::string;

struct Waypoint
{
    PoseStamped pose;
    double stay_time;
};

int main(int argc, char** argv)
{
#if ROSCONSOLE_MIN_SEVERITY == ROSCONSOLE_SEVERITY_DEBUG
    // Set the logging level manually to DEBUG
    ROSCONSOLE_AUTOINIT;
    log4cxx::LoggerPtr my_logger =
    log4cxx::Logger::getLogger( ROSCONSOLE_DEFAULT_NAME );
    my_logger->setLevel(
    ros::console::g_level_lookup[ros::console::levels::Debug]
    );

#endif
    ros::init(argc, argv, "dynamic_obstacle");
    ros::NodeHandle n("~");
    ros::Publisher object_pose_pub = n.advertise<geometry_msgs::PoseStamped>("/objectpose",10);
    std::vector<Waypoint> waypoints;
    {
        string obstacle_name;
        n.param<string>("name", obstacle_name, "obstacle");        
        ROS_DEBUG("Retriving waypoints for obstacle: %s", obstacle_name.c_str());
        XmlRpc::XmlRpcValue my_list;
        n.getParam("waypoints", my_list);
        ROS_ASSERT(my_list.getType() == XmlRpc::XmlRpcValue::TypeArray);
        for (int32_t i = 0; i < my_list.size(); ++i)
        {
            ROS_ASSERT(my_list[i].getType() == XmlRpc::XmlRpcValue::TypeArray);
            ROS_ASSERT(my_list[i].size() == 4);
            Waypoint waypoint;
            waypoint.pose.pose.position.x = static_cast<double>(my_list[i][0]);
            waypoint.pose.pose.position.y = static_cast<double>(my_list[i][1]);
            double yaw = M_PI/180.0 * static_cast<double>(my_list[i][2]);
            waypoint.pose.pose.orientation = tf::createQuaternionMsgFromYaw(yaw);
            waypoint.stay_time = static_cast<double>(my_list[i][3]);
            double scale_stay_time;
            n.param<double>("scale_stay_time", scale_stay_time, 1.0);
            waypoint.stay_time *= scale_stay_time;

            waypoint.pose.header.frame_id = obstacle_name;
            waypoints.push_back(waypoint);
            ROS_DEBUG("Obstacle waits at; (%f, %f, %f), for %f seconds",waypoint.pose.pose.position.x, waypoint.pose.pose.position.y, yaw, waypoint.stay_time);
        }
    }
    unsigned waypoint_i = 0;
    while (n.ok()) {
        waypoints[waypoint_i].pose.header.stamp = ros::Time::now();
        object_pose_pub.publish(waypoints[waypoint_i].pose);
        ros::Duration d(waypoints[waypoint_i].stay_time);
        d.sleep();
        waypoint_i++;
        waypoint_i %= waypoints.size();
    }
    return 0;
}
