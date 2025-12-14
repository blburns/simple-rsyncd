/*
 * Copyright 2024 SimpleDaemons
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include "simple-rsyncd/core/protocol.hpp"
#include <sstream>

namespace simple_rsyncd {

class ProtocolTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser_ = std::make_unique<ProtocolParser>();
    }

    std::unique_ptr<ProtocolParser> parser_;
};

TEST_F(ProtocolTest, ParseCommand) {
    std::string data = "LIST test /path\n";
    ProtocolMessage msg = parser_->parse(data);
    
    EXPECT_TRUE(msg.valid);
    EXPECT_EQ(msg.command, ProtocolCommand::LIST);
    EXPECT_EQ(msg.module, "test");
    EXPECT_EQ(msg.path, "/path");
}

TEST_F(ProtocolTest, ParseGet) {
    std::string data = "GET test /file.txt\n";
    ProtocolMessage msg = parser_->parse(data);
    
    EXPECT_TRUE(msg.valid);
    EXPECT_EQ(msg.command, ProtocolCommand::GET);
    EXPECT_EQ(msg.module, "test");
    EXPECT_EQ(msg.path, "/file.txt");
}

TEST_F(ProtocolTest, ParsePut) {
    std::string data = "PUT test /file.txt\n";
    ProtocolMessage msg = parser_->parse(data);
    
    EXPECT_TRUE(msg.valid);
    EXPECT_EQ(msg.command, ProtocolCommand::PUT);
    EXPECT_EQ(msg.module, "test");
    EXPECT_EQ(msg.path, "/file.txt");
}

TEST_F(ProtocolTest, ParseDelete) {
    std::string data = "DELETE test /file.txt\n";
    ProtocolMessage msg = parser_->parse(data);
    
    EXPECT_TRUE(msg.valid);
    EXPECT_EQ(msg.command, ProtocolCommand::DELETE);
    EXPECT_EQ(msg.module, "test");
    EXPECT_EQ(msg.path, "/file.txt");
}

TEST_F(ProtocolTest, ParseWithHeader) {
    std::string data = "@RSYNCD: 30\nLIST test /path\n";
    ProtocolMessage msg = parser_->parse(data);
    
    EXPECT_TRUE(msg.valid);
    EXPECT_EQ(msg.command, ProtocolCommand::LIST);
}

TEST_F(ProtocolTest, BuildResponse) {
    std::string response = parser_->buildResponse(true, "Success");
    EXPECT_NE(response.find("OK"), std::string::npos);
    EXPECT_NE(response.find("Success"), std::string::npos);
}

TEST_F(ProtocolTest, BuildErrorResponse) {
    std::string response = parser_->buildErrorResponse(1, "Error message");
    EXPECT_NE(response.find("ERROR"), std::string::npos);
    EXPECT_NE(response.find("Error message"), std::string::npos);
}

TEST_F(ProtocolTest, InvalidMessage) {
    std::string data = "INVALID COMMAND\n";
    ProtocolMessage msg = parser_->parse(data);
    
    EXPECT_FALSE(msg.valid);
    EXPECT_EQ(msg.command, ProtocolCommand::UNKNOWN);
}

} // namespace simple_rsyncd
