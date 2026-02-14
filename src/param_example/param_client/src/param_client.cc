/******************************************************************************
 * Copyright (c) 2022-2026 SEGAR. All Rights Reserved.
 * SPDX-License-Identifier: LicenseRef-Segar-Proprietary
 *
 * PROPRIETARY AND CONFIDENTIAL. See ./LICENSE
 * for license terms and restrictions.
 *****************************************************************************/

#include <memory>
#include <string>
#include <vector>

#include "param_example.pb.h"

#include "segar/parameter/segar_parameter_api.h"
#include "segar/segar.h"

int main(int argc, char* argv[]) {
  RETURN_VAL_IF(!rti::segar::Init(argv[0]), EXIT_FAILURE);

  auto node = rti::segar::CreateNode("param_client");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  using Parameter = rti::segar::Parameter;

  AINFO << "Parameter client started successfully";
  AINFO << "=== Testing Remote Parameter API ===";

  const std::string node_name = "param_server";

  // Get remote parameters.
  int int_val = 0;
  RETURN_VAL_IF(!Segar_Get_Remote_Param(node_name, "p1_int", &int_val),
                EXIT_FAILURE);
  AINFO << "p1_int: " << int_val;

  std::string str_val;
  RETURN_VAL_IF(!Segar_Get_Remote_Param(node_name, "p2_string", &str_val),
                EXIT_FAILURE);
  AINFO << "p2_string: " << str_val;

  param::example::Header pb_rcv;
  RETURN_VAL_IF(!Segar_Get_Remote_Param(node_name, "p3_pb", &pb_rcv),
                EXIT_FAILURE);
  AINFO << "p3_pb: " << pb_rcv.DebugString();

  auto header_sp = std::make_shared<param::example::Header>();
  RETURN_VAL_IF(!Segar_Get_Remote_Param(node_name, "p3_pb", header_sp),
                EXIT_FAILURE);
  AINFO << "header_sp: " << header_sp->DebugString();

  // Load remote parameters.
  RETURN_VAL_IF(
      !Segar_Load_Remote_Params(node_name, "/tmp/param_server.params"),
      EXIT_FAILURE);

  // List remote parameters.
  std::vector<Parameter> parameter_list;
  RETURN_VAL_IF(!Segar_List_Remote_Params(node_name, &parameter_list),
                EXIT_FAILURE);
  AINFO << "Remote parameters count: " << parameter_list.size();
  for (const auto& param : parameter_list) {
    AINFO << "param: " << param.DebugString();
  }

  // Dump remote parameters.
  RETURN_VAL_IF(
      !Segar_Dump_Remote_Params(node_name, "/tmp/param_server.params"),
      EXIT_FAILURE);
  AINFO << "Remote parameters dumped to /tmp/param_server.params";

  rti::segar::WaitForShutdown();
  return EXIT_SUCCESS;
}
