/****************************************************
 *
 * ****     ***     ****    *   *    *****    *****
 * *  *    *   *   *        ****     ***        *
 * *   *    ***     ****    *   *    *****      *
 *
 * student_server.h
 * 2024-07-24 02:16:14
 * Generated by rocket framework rocket_generator.py
 * Do not edit !!!
****************************************************/


#ifndef STUDENT_SERVER_SERVICE_STUDENT_SERVER_H 
#define STUDENT_SERVER_SERVICE_STUDENT_SERVER_H

#include <google/protobuf/service.h>
#include "student_server/pb/student_server.pb.h"

namespace student_server {

class StudentServiceImpl : public StudentService {

 public:

  StudentServiceImpl() = default;

  ~StudentServiceImpl() = default;

  // override from StudentService
  virtual void add_student(::google::protobuf::RpcController* controller,
                       const ::addStudentRequest* request,
                       ::addStudentResponse* response,
                       ::google::protobuf::Closure* done);

  // override from StudentService
  virtual void query_student(::google::protobuf::RpcController* controller,
                       const ::queryStudentRequest* request,
                       ::queryStudentResponse* response,
                       ::google::protobuf::Closure* done);

  

};

}


#endif