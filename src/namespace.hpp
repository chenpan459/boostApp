#pragma once

// Override at build time, e.g. -DNV_NAMESPACE=myapp
#ifndef NV_NAMESPACE
#define NV_NAMESPACE nvcomm
#endif

#define NV_NS_BEGIN namespace NV_NAMESPACE {
#define NV_NS_END }

#define NV_NS_CORE_BEGIN namespace NV_NAMESPACE :: core {
#define NV_NS_CORE_END }

#define NV_NS_DOMAIN_BEGIN namespace NV_NAMESPACE :: domain {
#define NV_NS_DOMAIN_END }

#define NV_NS_INFRA_PERSISTENCE_BEGIN namespace NV_NAMESPACE :: infra :: persistence {
#define NV_NS_INFRA_PERSISTENCE_END }

#define NV_NS_INFRA_NET_BEGIN namespace NV_NAMESPACE :: infra :: net {
#define NV_NS_INFRA_NET_END }

#define NV_NS_SERVICE_BEGIN namespace NV_NAMESPACE :: service {
#define NV_NS_SERVICE_END }

#define NV_NS_PLATFORM_BEGIN namespace NV_NAMESPACE :: platform {
#define NV_NS_PLATFORM_END }

#define NV_NS ::NV_NAMESPACE
#define NV_NS_CORE ::NV_NAMESPACE::core
#define NV_NS_DOMAIN ::NV_NAMESPACE::domain
#define NV_NS_INFRA_PERSISTENCE ::NV_NAMESPACE::infra::persistence
#define NV_NS_INFRA_NET ::NV_NAMESPACE::infra::net
#define NV_NS_SERVICE ::NV_NAMESPACE::service
#define NV_NS_PLATFORM ::NV_NAMESPACE::platform
