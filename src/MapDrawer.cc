/**
* This file is part of OA-SLAM.
*
* Copyright (C) 2022 Matthieu Zins <matthieu.zins@inria.fr>
* (Inria, LORIA, Université de Lorraine)
* OA-SLAM is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* OA-SLAM is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with OA-SLAM. If not, see <http://www.gnu.org/licenses/>.
*/


/**
* This file is part of ORB-SLAM2.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
*
* ORB-SLAM2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
*/

#include "MapDrawer.h"
#include "MapPoint.h"
#include "KeyFrame.h"
#include "Ellipsoid.h"
#include "MapObject.h"
#include "ObjectTrack.h"
#include "ColorManager.h"
#include <pangolin/pangolin.h>
#include <mutex>
#include <unordered_map>

namespace ORB_SLAM2
{


MapDrawer::MapDrawer(Map* pMap, const string &strSettingPath):mpMap(pMap)
{
    cv::FileStorage fSettings(strSettingPath, cv::FileStorage::READ);

    mKeyFrameSize = fSettings["Viewer.KeyFrameSize"];
    mKeyFrameLineWidth = fSettings["Viewer.KeyFrameLineWidth"];
    mGraphLineWidth = fSettings["Viewer.GraphLineWidth"];
    mPointSize = fSettings["Viewer.PointSize"];
    mPointSize = 2;
    mCameraSize = fSettings["Viewer.CameraSize"];
    mCameraLineWidth = fSettings["Viewer.CameraLineWidth"];
    use_category_cols_ = false;
}

void MapDrawer::DrawMapPoints(double size, bool ignore_objects_points)
{
    const vector<MapPoint*> &vpMPs = mpMap->GetAllMapPoints();
    const vector<MapPoint*> &vpRefMPs = mpMap->GetReferenceMapPoints();

    set<MapPoint*> spRefMPs(vpRefMPs.begin(), vpRefMPs.end());

    if(vpMPs.empty())
        return;

    std::unordered_set<MapPoint*> associated;
    if (ignore_objects_points) {
        const std::vector<MapObject*> objects = mpMap->GetAllMapObjects();
        for (auto* obj : objects) {
            auto assoc_points = obj->GetTrack()->GetFilteredAssociatedMapPoints(10);
            for (auto pt_cnt : assoc_points) {
                associated.insert(pt_cnt.first);
            }
        }
    }

    glPointSize(size);
    glBegin(GL_POINTS);
    glColor3f(0.0,0.0,0.0);

    for(size_t i=0, iend=vpMPs.size(); i<iend;i++)
    {
        if(vpMPs[i]->isBad() || spRefMPs.count(vpMPs[i]))
            continue;
        if (associated.count(vpMPs[i]) > 0)
            continue;
        cv::Mat pos = vpMPs[i]->GetWorldPos();
        glVertex3f(pos.at<float>(0),pos.at<float>(1),pos.at<float>(2));
    }
    glEnd();

    glPointSize(size);
    glBegin(GL_POINTS);
    glColor3f(1.0,0.0,0.0);

    for(set<MapPoint*>::iterator sit=spRefMPs.begin(), send=spRefMPs.end(); sit!=send; sit++)
    {
        if((*sit)->isBad())
            continue;
        if (associated.count(*sit) > 0)
            continue;
        cv::Mat pos = (*sit)->GetWorldPos();
        glVertex3f(pos.at<float>(0),pos.at<float>(1),pos.at<float>(2));

    }

    glEnd();
}

void MapDrawer::DrawMapObjectsPoints(double size)
{
    const vector<MapPoint*> &vpMPs = mpMap->GetAllMapPoints();
    const vector<MapPoint*> &vpRefMPs = mpMap->GetReferenceMapPoints();

    set<MapPoint*> spRefMPs(vpRefMPs.begin(), vpRefMPs.end());

    if(vpMPs.empty())
        return;

    const auto& color_manager = CategoryColorsManager::GetInstance();
    const std::vector<MapObject*> objects = mpMap->GetAllMapObjects();
    for (auto* obj : objects) {
        cv::Scalar c;
        if (use_category_cols_) {
            c = color_manager[obj->GetTrack()->GetCategoryId()];
        } else {
            c = obj->GetTrack()->GetColor();
        }
        glColor3f(static_cast<double>(c(2)) / 255,
                static_cast<double>(c(1)) / 255,
                static_cast<double>(c(0)) / 255);
        auto assoc_points = obj->GetTrack()->GetFilteredAssociatedMapPoints(10);
        glPointSize(size);
        glBegin(GL_POINTS);
        for (auto pt_cnt : assoc_points) {
            MapPoint* pt = pt_cnt.first;

            if (pt->isBad())
                continue;
            cv::Mat pos = pt->GetWorldPos();
            glVertex3f(pos.at<float>(0),pos.at<float>(1),pos.at<float>(2));
        }
        glEnd();
    }
}

void MapDrawer::DrawDistanceEstimation(double depth, cv::Mat &Tcw)
{
    if (Tcw.cols != 4)
        return;
    Eigen::Matrix4d T = cvToEigenMatrix<double, float, 4, 4>(Tcw);
    Eigen::Matrix3d R = T.block<3, 3>(0, 0);
    Eigen::Vector3d t = T.block<3, 1>(0, 3);
    Eigen::Matrix3d o = R.transpose();
    Eigen::Vector3d p = -o * t;
    Eigen::Vector3d e = p + depth * o.col(2);

    glColor3f(0.0, 0.0, 1.0);
    glBegin(GL_LINE_STRIP);
    glVertex3f(p[0], p[1], p[2]);
    glVertex3f(e[0], e[1], e[2]);
    glEnd();
}

void MapDrawer::DrawKeyFrames(const bool bDrawKF, const bool bDrawGraph)
{
    const float &w = mKeyFrameSize;
    const float h = w*0.75;
    const float z = w*0.6;

    const vector<KeyFrame*> vpKFs = mpMap->GetAllKeyFrames();

    if(bDrawKF)
    {
        for(size_t i=0; i<vpKFs.size(); i++)
        {
            KeyFrame* pKF = vpKFs[i];
            cv::Mat Twc = pKF->GetPoseInverse().t();

            glPushMatrix();

            glMultMatrixf(Twc.ptr<GLfloat>(0));

            glLineWidth(mKeyFrameLineWidth);
            glColor3f(0.0f,0.0f,1.0f);
            glBegin(GL_LINES);
            glVertex3f(0,0,0);
            glVertex3f(w,h,z);
            glVertex3f(0,0,0);
            glVertex3f(w,-h,z);
            glVertex3f(0,0,0);
            glVertex3f(-w,-h,z);
            glVertex3f(0,0,0);
            glVertex3f(-w,h,z);

            glVertex3f(w,h,z);
            glVertex3f(w,-h,z);

            glVertex3f(-w,h,z);
            glVertex3f(-w,-h,z);

            glVertex3f(-w,h,z);
            glVertex3f(w,h,z);

            glVertex3f(-w,-h,z);
            glVertex3f(w,-h,z);
            glEnd();

            glPopMatrix();
        }
    }


    if(bDrawGraph)
    {
        glLineWidth(mGraphLineWidth);
        glColor4f(0.0f,1.0f,0.0f,0.6f);
        glBegin(GL_LINES);

        for(size_t i=0; i<vpKFs.size(); i++)
        {
            // Covisibility Graph
            const vector<KeyFrame*> vCovKFs = vpKFs[i]->GetCovisiblesByWeight(100);
            cv::Mat Ow = vpKFs[i]->GetCameraCenter();
            if(!vCovKFs.empty())
            {
                for(vector<KeyFrame*>::const_iterator vit=vCovKFs.begin(), vend=vCovKFs.end(); vit!=vend; vit++)
                {
                    if((*vit)->mnId<vpKFs[i]->mnId)
                        continue;
                    cv::Mat Ow2 = (*vit)->GetCameraCenter();
                    glVertex3f(Ow.at<float>(0),Ow.at<float>(1),Ow.at<float>(2));
                    glVertex3f(Ow2.at<float>(0),Ow2.at<float>(1),Ow2.at<float>(2));
                }
            }

            // Spanning tree
            KeyFrame* pParent = vpKFs[i]->GetParent();
            if(pParent)
            {
                cv::Mat Owp = pParent->GetCameraCenter();
                glVertex3f(Ow.at<float>(0),Ow.at<float>(1),Ow.at<float>(2));
                glVertex3f(Owp.at<float>(0),Owp.at<float>(1),Owp.at<float>(2));
            }

            // Loops
            set<KeyFrame*> sLoopKFs = vpKFs[i]->GetLoopEdges();
            for(set<KeyFrame*>::iterator sit=sLoopKFs.begin(), send=sLoopKFs.end(); sit!=send; sit++)
            {
                if((*sit)->mnId<vpKFs[i]->mnId)
                    continue;
                cv::Mat Owl = (*sit)->GetCameraCenter();
                glVertex3f(Ow.at<float>(0),Ow.at<float>(1),Ow.at<float>(2));
                glVertex3f(Owl.at<float>(0),Owl.at<float>(1),Owl.at<float>(2));
            }
        }

        glEnd();
    }
}

void MapDrawer::DrawCurrentCamera(pangolin::OpenGlMatrix &Twc)
{
    const float &w = mCameraSize;
    const float h = w*0.75;
    const float z = w*0.6;

    glPushMatrix();

#ifdef HAVE_GLES
        glMultMatrixf(Twc.m);
#else
        glMultMatrixd(Twc.m);
#endif

    glLineWidth(mCameraLineWidth);
    glColor3f(0.0f,1.0f,0.0f);
    glBegin(GL_LINES);
    glVertex3f(0,0,0);
    glVertex3f(w,h,z);
    glVertex3f(0,0,0);
    glVertex3f(w,-h,z);
    glVertex3f(0,0,0);
    glVertex3f(-w,-h,z);
    glVertex3f(0,0,0);
    glVertex3f(-w,h,z);

    glVertex3f(w,h,z);
    glVertex3f(w,-h,z);

    glVertex3f(-w,h,z);
    glVertex3f(-w,-h,z);

    glVertex3f(-w,h,z);
    glVertex3f(w,h,z);

    glVertex3f(-w,-h,z);
    glVertex3f(w,-h,z);
    glEnd();

    glPopMatrix();
}


void MapDrawer::SetCurrentCameraPose(const cv::Mat &Tcw)
{
    unique_lock<mutex> lock(mMutexCamera);
    mCameraPose = Tcw.clone();
}

void MapDrawer::GetCurrentOpenGLCameraMatrix(pangolin::OpenGlMatrix &M)
{
    if(!mCameraPose.empty())
    {
        cv::Mat Rwc(3,3,CV_32F);
        cv::Mat twc(3,1,CV_32F);
        {
            unique_lock<mutex> lock(mMutexCamera);
            Rwc = mCameraPose.rowRange(0,3).colRange(0,3).t();
            twc = -Rwc*mCameraPose.rowRange(0,3).col(3);
        }

        M.m[0] = Rwc.at<float>(0,0);
        M.m[1] = Rwc.at<float>(1,0);
        M.m[2] = Rwc.at<float>(2,0);
        M.m[3]  = 0.0;

        M.m[4] = Rwc.at<float>(0,1);
        M.m[5] = Rwc.at<float>(1,1);
        M.m[6] = Rwc.at<float>(2,1);
        M.m[7]  = 0.0;

        M.m[8] = Rwc.at<float>(0,2);
        M.m[9] = Rwc.at<float>(1,2);
        M.m[10] = Rwc.at<float>(2,2);
        M.m[11]  = 0.0;

        M.m[12] = twc.at<float>(0);
        M.m[13] = twc.at<float>(1);
        M.m[14] = twc.at<float>(2);
        M.m[15]  = 1.0;
    }
    else
        M.SetIdentity();
}


void MapDrawer::DrawMapObjects()
{
    const std::vector<MapObject*> objects = mpMap->GetAllMapObjects();

    glPointSize(mPointSize);
    const auto& color_manager = CategoryColorsManager::GetInstance();
    glLineWidth(2);
    for (auto *obj : objects) {
        cv::Scalar c;
        if (use_category_cols_) {
            c = color_manager[obj->GetTrack()->GetCategoryId()];
        } else {
            c = obj->GetTrack()->GetColor();
        }
        glColor3f(static_cast<double>(c(2)) / 255,
                  static_cast<double>(c(1)) / 255,
                  static_cast<double>(c(0)) / 255);
        const Ellipsoid& ell = obj->GetEllipsoid();
        if (!display_3d_bbox_) {
            auto pts = ell.GeneratePointCloud();
            int i = 0;
            while (i < pts.rows()) {
                glBegin(GL_LINE_STRIP);
                // glColor3f(0.0, 1.0, 0.0);
                // glBegin(GL_POINTS);
                for (int k = 0; k < 50; ++k, ++i){
                    glVertex3f(pts(i, 0), pts(i, 1), pts(i, 2));
                }
                glEnd();
            }
        } else {
            Eigen::Vector3d center = ell.GetCenter();
            Eigen::Vector3d axes = ell.GetAxes();
            Eigen::Matrix3d R = ell.GetOrientation();
            Eigen::Matrix<double, 8, 3> pts;
            pts << -axes[0], -axes[1], -axes[2],
                    axes[0], -axes[1], -axes[2],
                    axes[0],  axes[1], -axes[2],
                   -axes[0],  axes[1], -axes[2],
                   -axes[0], -axes[1],  axes[2],
                    axes[0], -axes[1],  axes[2],
                    axes[0],  axes[1],  axes[2],
                   -axes[0],  axes[1],  axes[2];
            Eigen::Matrix<double, 8, 3> obb = (R * pts.transpose()).transpose();
            obb.rowwise() += center.transpose();

            glBegin(GL_LINE_STRIP);
            glVertex3f(obb(0, 0), obb(0, 1), obb(0, 2));
            glVertex3f(obb(1, 0), obb(1, 1), obb(1, 2));
            glVertex3f(obb(2, 0), obb(2, 1), obb(2, 2));
            glVertex3f(obb(3, 0), obb(3, 1), obb(3, 2));
            glVertex3f(obb(0, 0), obb(0, 1), obb(0, 2));
            glVertex3f(obb(4, 0), obb(4, 1), obb(4, 2));
            glVertex3f(obb(5, 0), obb(5, 1), obb(5, 2));
            glVertex3f(obb(1, 0), obb(1, 1), obb(1, 2));
            glVertex3f(obb(5, 0), obb(5, 1), obb(5, 2));
            glVertex3f(obb(6, 0), obb(6, 1), obb(6, 2));
            glVertex3f(obb(2, 0), obb(2, 1), obb(2, 2));
            glVertex3f(obb(6, 0), obb(6, 1), obb(6, 2));
            glVertex3f(obb(7, 0), obb(7, 1), obb(7, 2));
            glVertex3f(obb(3, 0), obb(3, 1), obb(3, 2));
            glVertex3f(obb(7, 0), obb(7, 1), obb(7, 2));
            glVertex3f(obb(4, 0), obb(4, 1), obb(4, 2));
            glEnd();
        }
    }
}


} //namespace ORB_SLAM
