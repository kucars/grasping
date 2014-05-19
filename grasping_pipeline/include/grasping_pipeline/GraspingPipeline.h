#include <ros/ros.h>
#include <actionlib/client/simple_action_client.h>
#include <actionlib/client/terminal_state.h>
#include <perception_msgs/DetectObjectsAction.h>
#include <ist_grasp_generation_msgs/GenerateGraspsAction.h>

#include <ist_grasp_generation_msgs/GraspingAction.h>
#include <actionlib/server/simple_action_server.h>

class GraspingPipelineAction
{
protected:

    ros::NodeHandle nh_;
    // NodeHandle instance must be created before this line. Otherwise strange error may occur.
    actionlib::SimpleActionServer<ist_grasp_generation_msgs::GraspingAction> as_;
    std::string action_name_;
    // create messages that are used to published feedback/result
    ist_grasp_generation_msgs::GraspingFeedback feedback_;
    ist_grasp_generation_msgs::GraspingResult result_;

public:

    GraspingPipelineAction(std::string name) :
        as_(nh_, name, boost::bind(&GraspingPipelineAction::executeCB, this, _1), false),
        action_name_(name)
    {
        as_.start();
    }

    ~GraspingPipelineAction(void)
    {
    }

    void executeCB(const ist_grasp_generation_msgs::GraspingGoalConstPtr &goal)
    {
        // helper variables
        ros::Rate r(1);
        bool success = true;

        // push_back the seeds for the fibonacci sequence
        //feedback_.sequence.clear();
        //feedback_.sequence.push_back(0);
        //feedback_.sequence.push_back(1);

        // publish info to the console for the user
        //ROS_INFO("%s: Executing, creating fibonacci sequence of order %i with seeds %i, %i", action_name_.c_str(), goal->order, feedback_.sequence[0], feedback_.sequence[1]);

        ////////////////////
        // DETECT OBJECTS //
        ////////////////////

        actionlib::SimpleActionClient<perception_msgs::DetectObjectsAction> detect_objects("detect_objects_server", true);

        ROS_INFO("Waiting for object detection action server to start.");
        // wait for the action server to start
        detect_objects.waitForServer(); //will wait for infinite time

        // send a goal to the action
        perception_msgs::DetectObjectsGoal object_detection_goal;

        object_detection_goal.table_region.x_filter_min = 0.2;
        object_detection_goal.table_region.x_filter_max = 1.0;
        object_detection_goal.table_region.y_filter_min =-0.4;
        object_detection_goal.table_region.y_filter_max = 0.4;
        object_detection_goal.table_region.z_filter_min =-0.1;
        object_detection_goal.table_region.z_filter_max = 0.3;

        detect_objects.sendGoal(object_detection_goal);

        //wait for the action to return
        bool finished_before_timeout = detect_objects.waitForResult(ros::Duration(30.0));

        if (finished_before_timeout)
        {
            actionlib::SimpleClientGoalState state = detect_objects.getState();
            ROS_INFO("Action finished: %s",state.toString().c_str());
        }
        else
        {
            ROS_INFO("Action did not finish before the time out.");
            as_.setPreempted();
            success = false;
            return;
        }

        /////////////////////
        // GENERATE GRASPS //
        /////////////////////

        actionlib::SimpleActionClient<ist_grasp_generation_msgs::GenerateGraspsAction> generate_grasps("generate_grasps_server", true);

        ROS_INFO("Waiting for generate grasps action server to start.");
        // wait for the action server to start
        generate_grasps.waitForServer(); //will wait for infinite time


        ist_grasp_generation_msgs::GenerateGraspsGoal grasps_goal;

        grasps_goal.object_to_grasp_id=0;
        grasps_goal.object_list=detect_objects.getResult()->object_list;
        generate_grasps.sendGoal(grasps_goal);

        //wait for the action to return
        finished_before_timeout = detect_objects.waitForResult(ros::Duration(30.0));

        if (finished_before_timeout)
        {
            actionlib::SimpleClientGoalState state = generate_grasps.getState();
            ROS_INFO("Action finished: %s",state.toString().c_str());
        }
        else
        {
            ROS_INFO("Action did not finish before the time out.");
            as_.setPreempted();
            success = false;
        }

        if(success)
        {
            //result_.sequence = feedback_.state;
            ROS_INFO("%s: Succeeded", action_name_.c_str());
            // set the action state to succeeded
            as_.setSucceeded(result_);
        }
    }
};
