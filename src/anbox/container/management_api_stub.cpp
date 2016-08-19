/*
 * Copyright (C) 2016 Simon Fels <morphis@gravedo.de>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "anbox/container/management_api_stub.h"
#include "anbox/rpc/channel.h"
#include "anbox/logger.h"

#include "anbox_rpc.pb.h"
#include "anbox_container.pb.h"

namespace anbox {
namespace container {
ManagementApiStub::ManagementApiStub(const std::shared_ptr<rpc::Channel> &channel) :
    channel_(channel) {
}

ManagementApiStub::~ManagementApiStub() {
}

void ManagementApiStub::start_container(const Configuration &configuration) {
    auto c = std::make_shared<Request<protobuf::rpc::Void>>();

    protobuf::container::StartContainer message;
    auto message_configuration = new protobuf::container::Configuration;

    for (const auto item : configuration.bind_mounts) {
        auto bind_mount_message = message_configuration->add_bind_mounts();
        bind_mount_message->set_source(item.first);
        bind_mount_message->set_target(item.second);
    }

    message.set_allocated_configuration(message_configuration);

    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        start_wait_handle_.expect_result();
    }

    channel_->call_method("start_container",
                          &message,
                          c->response.get(),
                          google::protobuf::NewCallback(this, &ManagementApiStub::container_started, c.get()));

    start_wait_handle_.wait_for_all();

    if (c->response->has_error())
        throw std::runtime_error(c->response->error());
}

void ManagementApiStub::container_started(Request<protobuf::rpc::Void> *request) {
    (void) request;
    DEBUG("");
    start_wait_handle_.result_received();
}
} // namespace container
} // namespace anbox