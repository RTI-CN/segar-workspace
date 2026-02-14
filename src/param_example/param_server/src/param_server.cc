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

  auto node = rti::segar::CreateNode("param_server");
  RETURN_VAL_IF(!node, EXIT_FAILURE);

  using Parameter = rti::segar::Parameter;

  // Load local parameters.
  RETURN_VAL_IF(!Segar_Load_Local_Params(node, "config/params.yaml"),
                EXIT_FAILURE);
  AINFO << "Parameter server started successfully";

  AINFO << "=== Testing Local Parameter API ===";

  // List local parameters.
  std::vector<Parameter> parameter_list;
  RETURN_VAL_IF(!Segar_List_Local_Params(node, &parameter_list), EXIT_FAILURE);
  AINFO << "Initial local parameters count: " << parameter_list.size();
  for (const auto& param : parameter_list) {
    AINFO << "param: " << param.DebugString();
  }

  // Set local parameters.
  RETURN_VAL_IF(!Segar_Set_Local_Param(node, "p1_int", 1), EXIT_FAILURE);
  RETURN_VAL_IF(!Segar_Set_Local_Param(node, "p2_string", "test"),
                EXIT_FAILURE);

  param::example::Header header;
  header.set_module_name("param_server");
  header.set_timestamp_sec(1234.56);
  header.set_sequence_num(1);
  RETURN_VAL_IF(!Segar_Set_Local_Param(node, "p3_pb", header), EXIT_FAILURE);

  // Get local parameters.
  int int_val = 0;
  RETURN_VAL_IF(!Segar_Get_Local_Param(node, "p1_int", &int_val), EXIT_FAILURE);
  AINFO << "p1_int: " << int_val;

  std::string str_val;
  RETURN_VAL_IF(!Segar_Get_Local_Param(node, "p2_string", &str_val),
                EXIT_FAILURE);
  AINFO << "p2_string: " << str_val;

  param::example::Header pb_rcv;
  RETURN_VAL_IF(!Segar_Get_Local_Param(node, "p3_pb", &pb_rcv), EXIT_FAILURE);
  AINFO << "p3_pb: " << pb_rcv.DebugString();

  auto header_sp = std::make_shared<param::example::Header>();
  RETURN_VAL_IF(!Segar_Get_Local_Param(node, "p3_pb", header_sp), EXIT_FAILURE);
  AINFO << "header_sp: " << header_sp->DebugString();

  // List local parameters again.
  RETURN_VAL_IF(!Segar_List_Local_Params(node, &parameter_list), EXIT_FAILURE);
  AINFO << "After setting, local parameters count: " << parameter_list.size();
  for (const auto& param : parameter_list) {
    AINFO << "param: " << param.DebugString();
  }

  // Dump local parameters.
  RETURN_VAL_IF(!Segar_Dump_Local_Params(node, "/tmp/param_server.params"),
                EXIT_FAILURE);
  AINFO << "Local parameters dumped to /tmp/param_server.params";

  rti::segar::WaitForShutdown();
  return EXIT_SUCCESS;
}
