# Error Handling Guide

This guide covers the comprehensive error handling system introduced in v0.3.0.

## Overview

Simple RSync Daemon v0.3.0 includes a robust error handling system with:
- **Error Categories**: Organized error types by domain
- **Error Codes**: Numeric codes for programmatic handling
- **Error Context**: Rich contextual information
- **Error Recovery**: Suggestions for resolving errors
- **Structured Error Reporting**: JSON and text formats

## Error Categories

Errors are organized into the following categories:

| Category | Code Range | Description |
|----------|------------|-------------|
| **CONFIGURATION** | 1000-1999 | Configuration file and validation errors |
| **AUTHENTICATION** | 2000-2999 | User authentication failures |
| **AUTHORIZATION** | 3000-3999 | Access control and permission errors |
| **NETWORK** | 4000-4999 | Network connection and socket errors |
| **FILE_SYSTEM** | 5000-5999 | File and directory operation errors |
| **PROTOCOL** | 6000-6999 | RSync protocol parsing errors |
| **MODULE** | 7000-7999 | Module-specific errors |
| **INTERNAL** | 8000-8999 | Internal daemon errors |
| **EXTERNAL** | 9000-9999 | External dependency errors |

## Error Codes

### Configuration Errors (1000-1999)

| Code | Name | Description |
|------|------|-------------|
| 1001 | `CONFIG_FILE_NOT_FOUND` | Configuration file does not exist |
| 1002 | `CONFIG_INVALID_FORMAT` | Configuration file has invalid syntax |
| 1003 | `CONFIG_VALIDATION_FAILED` | Configuration validation failed |
| 1004 | `CONFIG_RELOAD_FAILED` | Configuration reload failed |

### Authentication Errors (2000-2999)

| Code | Name | Description |
|------|------|-------------|
| 2001 | `AUTH_FAILED` | Authentication failed |
| 2002 | `AUTH_USER_NOT_FOUND` | User does not exist |
| 2003 | `AUTH_PASSWORD_INVALID` | Invalid password |
| 2004 | `AUTH_ACCOUNT_LOCKED` | Account is locked |
| 2005 | `AUTH_PASSWORD_EXPIRED` | Password has expired |
| 2006 | `AUTH_SESSION_EXPIRED` | Session has expired |

### Authorization Errors (3000-3999)

| Code | Name | Description |
|------|------|-------------|
| 3001 | `AUTHZ_ACCESS_DENIED` | Access denied |
| 3002 | `AUTHZ_INSUFFICIENT_PERMISSIONS` | Insufficient permissions |
| 3003 | `AUTHZ_IP_DENIED` | IP address denied |

### Network Errors (4000-4999)

| Code | Name | Description |
|------|------|-------------|
| 4001 | `NETWORK_CONNECTION_FAILED` | Connection failed |
| 4002 | `NETWORK_BIND_FAILED` | Failed to bind to address/port |
| 4003 | `NETWORK_LISTEN_FAILED` | Failed to listen on socket |
| 4004 | `NETWORK_ACCEPT_FAILED` | Failed to accept connection |
| 4005 | `NETWORK_TIMEOUT` | Network operation timed out |

### File System Errors (5000-5999)

| Code | Name | Description |
|------|------|-------------|
| 5001 | `FS_FILE_NOT_FOUND` | File does not exist |
| 5002 | `FS_PERMISSION_DENIED` | Permission denied |
| 5003 | `FS_DISK_FULL` | Disk is full |
| 5004 | `FS_PATH_TRAVERSAL` | Path traversal attempt detected |
| 5005 | `FS_INVALID_PATH` | Invalid file path |

### Protocol Errors (6000-6999)

| Code | Name | Description |
|------|------|-------------|
| 6001 | `PROTOCOL_INVALID_MESSAGE` | Invalid protocol message |
| 6002 | `PROTOCOL_UNSUPPORTED_VERSION` | Unsupported protocol version |
| 6003 | `PROTOCOL_PARSE_ERROR` | Protocol parsing error |

### Module Errors (7000-7999)

| Code | Name | Description |
|------|------|-------------|
| 7001 | `MODULE_NOT_FOUND` | Module does not exist |
| 7002 | `MODULE_INVALID_CONFIG` | Invalid module configuration |
| 7003 | `MODULE_OPERATION_FAILED` | Module operation failed |

### Internal Errors (8000-8999)

| Code | Name | Description |
|------|------|-------------|
| 8001 | `INTERNAL_ERROR` | Internal error |
| 8002 | `INTERNAL_MEMORY_ERROR` | Memory allocation error |
| 8003 | `INTERNAL_THREAD_ERROR` | Thread operation error |

### External Errors (9000-9999)

| Code | Name | Description |
|------|------|-------------|
| 9001 | `EXTERNAL_DEPENDENCY_ERROR` | External dependency error |
| 9002 | `EXTERNAL_SSL_ERROR` | SSL/TLS error |

## Using Error Handling

### Catching Errors

```cpp
#include "simple-rsyncd/core/error.hpp"

try {
    // Operation that may fail
    daemon.start();
} catch (const RSyncError& e) {
    // Handle error
    std::cerr << "Error: " << e.toString() << std::endl;
    std::cerr << "Code: " << static_cast<int>(e.getCode()) << std::endl;
    std::cerr << "Category: " << e.getCategory() << std::endl;
    std::cerr << "Recovery: " << e.getRecoverySuggestion() << std::endl;
}
```

### Creating Errors with Context

```cpp
#include "simple-rsyncd/core/error.hpp"

// Simple error
throw RSyncError(
    ErrorCode::AUTH_FAILED,
    "Authentication failed",
    ErrorCategory::AUTHENTICATION
);

// Error with context
ErrorContext ctx;
ctx.setComponent("AuthenticationManager")
   .setOperation("authenticate")
   .setUser("admin")
   .setClientAddress("192.168.1.100");

throw RSyncError(
    ErrorCode::AUTH_PASSWORD_INVALID,
    "Invalid password",
    ctx.build(),
    ErrorCategory::AUTHENTICATION
);
```

### Error Context Builder

The `ErrorContext` class provides a fluent interface for building error context:

```cpp
ErrorContext ctx;
ctx.setComponent("Module")
   .setOperation("transferFile")
   .setPath("/var/data/file.txt")
   .setUser("admin")
   .setClientAddress("192.168.1.100")
   .addField("file_size", "1024")
   .addField("transfer_rate", "512");

std::string context = ctx.build();
// Result: "component=Module, operation=transferFile, path=/var/data/file.txt, ..."
```

## Error Recovery

Some errors are recoverable and can be retried:

```cpp
RSyncError error(ErrorCode::NETWORK_TIMEOUT, "Connection timed out");

if (error.isRecoverable()) {
    // Retry operation
    retryOperation();
} else {
    // Handle non-recoverable error
    handleError(error);
}
```

**Recoverable Errors:**
- `NETWORK_TIMEOUT`
- `NETWORK_CONNECTION_FAILED`
- `AUTH_SESSION_EXPIRED`
- `CONFIG_RELOAD_FAILED`

## Error Reporting Formats

### Text Format

```
[CONFIGURATION] [CONFIG_FILE_NOT_FOUND] Configuration file not found (Context: component=Configuration, operation=loadFromFile, path=/etc/simple-rsyncd/rsyncd.conf)
```

### JSON Format

```json
{
  "timestamp": "2024-12-14 10:30:45",
  "code": 1001,
  "code_string": "CONFIG_FILE_NOT_FOUND",
  "category": "CONFIGURATION",
  "message": "Configuration file not found",
  "context": "component=Configuration, operation=loadFromFile, path=/etc/simple-rsyncd/rsyncd.conf",
  "fields": {
    "component": "Configuration",
    "operation": "loadFromFile",
    "path": "/etc/simple-rsyncd/rsyncd.conf"
  }
}
```

## Error Logging

Errors are automatically logged with full context:

```cpp
try {
    // Operation
} catch (const RSyncError& e) {
    logger->error(e.toJSON());
    // Or
    logger->error(e.toString());
}
```

## Best Practices

1. **Always include context**: Use `ErrorContext` to provide detailed information
2. **Use appropriate error codes**: Choose the most specific error code
3. **Provide recovery suggestions**: Help users resolve errors
4. **Log errors appropriately**: Use structured logging for errors
5. **Handle recoverable errors**: Implement retry logic for recoverable errors

## Error Handling in Configuration

Configuration errors are automatically caught and reported:

```bash
# Configuration validation will show detailed errors
simple-rsyncd test --config /etc/simple-rsyncd/rsyncd.conf

# Example output:
# Error [CONFIGURATION] [CONFIG_VALIDATION_FAILED]: 
#   - Module 'backup': path '/var/backup' does not exist
#   - SSL certificate file '/etc/ssl/certs/server.crt' not found
```

## Troubleshooting with Errors

When troubleshooting, use error codes and categories to identify issues:

1. **Check error category**: Identifies the component with the problem
2. **Check error code**: Provides specific information about the failure
3. **Review error context**: Shows what operation was being performed
4. **Follow recovery suggestions**: Provides actionable steps to resolve

---

**For more information**, see the [Configuration Guide](configuration/README.md) and [Troubleshooting Guide](troubleshooting/README.md).
