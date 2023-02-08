/* Diagnostic Client library
 * Copyright (C) 2022  Avijit Dey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <gtest/gtest.h>
#include "main.h"
#include "include/create_diagnostic_client.h"
#include <chrono>

namespace doip_client {

TEST_F(DoipClientFixture, DISABLED_StartupAndTermination) {
  // Get conversation for tester one
  diag::client::conversation::DiagClientConversation &diag_client_conversation{
    GetDiagClientRef().GetDiagnosticClientConversation("DiagTesterOne")};

  diag_client_conversation.Startup();
  diag_client_conversation.Shutdown();
}

TEST_F(DoipClientFixture, SendVehicleIdentification) {

  std::this_thread::sleep_for(std::chrono::seconds(1));
  diag::client::vehicle_info::VehicleInfoListRequestType vehicle_info_request{0u, ""};
  GetDiagClientRef().SendVehicleIdentificationRequest(vehicle_info_request);
  std::this_thread::sleep_for(std::chrono::seconds(1));
}

} // doip_client