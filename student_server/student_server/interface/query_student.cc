/****************************************************
 *
 * ****     ***     ****    *   *    *****    *****
 * *  *    *   *   *        ****     ***        *
 * *   *    ***     ****    *   *    *****      *
 *
 * query_student.cc
 * 2024-07-24 02:16:14
 * Generated by rocket framework rocket_generator.py
 * File will not generate while exist
 * Allow editing
****************************************************/


#include <rocket/common/log.h>
#include "student_server/interface/query_student.h"
#include "student_server/interface/interface.h"
#include "student_server/pb/student_server.pb.h"

namespace student_server {

QueryStudentInterface::QueryStudentInterface(const ::queryStudentRequest* request, ::queryStudentResponse* response, 
    rocket::RpcClosure* done, rocket::RpcController* controller)
  : Interface(dynamic_cast<const google::protobuf::Message*>(request), dynamic_cast<google::protobuf::Message*>(response), done, controller),
    m_request(request), 
    m_response(response) {
  APPINFOLOG("In|request:{%s}", request->ShortDebugString().c_str());
}

QueryStudentInterface::~QueryStudentInterface() {
  APPINFOLOG("Out|response:{%s}", m_response->ShortDebugString().c_str());
}

void QueryStudentInterface::run() {

  //
  // Run your business logic at here
  // 

  m_response->set_ret_code(0);
  m_response->set_res_info("OK");

}

void QueryStudentInterface::setError(int code, const std::string& err_info) {
  m_response->set_ret_code(code);
  m_response->set_res_info(err_info);
}

}