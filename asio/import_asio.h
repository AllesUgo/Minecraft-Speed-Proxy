#pragma once
#define ASIO_STANDALONE  // 必须定义在包含 asio.hpp 之前

#include <asio.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/experimental/channel.hpp>
#include <asio/experimental/parallel_group.hpp>