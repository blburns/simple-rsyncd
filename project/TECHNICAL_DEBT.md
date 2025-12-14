# Simple RSync Daemon - Technical Debt

**Date:** December 2024
**Current Version:** 0.1.0
**Purpose:** Track technical debt, known issues, and areas requiring improvement

---

## 🎯 Overview

This document tracks technical debt for the simple-rsyncd project based on actual code review.

**Total Debt Items:** 15+
**Estimated Effort:** ~500-800 hours (to match documentation claims)

---

## 🔴 Critical Priority (Blocking MVP)

### 1. RSync Protocol Implementation
**Status:** ❌ **Not Started**
**Priority:** 🔴 **CRITICAL**
**Estimated Effort:** 100-150 hours

**Action Items:**
- [ ] Implement rsync protocol parser
- [ ] Implement protocol command handling
- [ ] Implement message parsing
- [ ] Implement protocol state machine
- [ ] Add protocol error handling

**Impact:** **Cannot function as rsync daemon without this**

**Target:** MVP release

---

### 2. File Transfer Engine
**Status:** ❌ **Not Started**
**Priority:** 🔴 **CRITICAL**
**Estimated Effort:** 50-75 hours

**Action Items:**
- [ ] Implement basic file transfer (no delta sync initially)
- [ ] Implement file operations (read, write, delete)
- [ ] Implement directory operations
- [ ] Implement file listing
- [ ] Add transfer error handling

**Impact:** **Cannot transfer files without this**

**Target:** MVP release

---

### 3. Configuration File Parsing
**Status:** ⚠️ **Stubbed**
**Priority:** 🔴 **CRITICAL**
**Estimated Effort:** 20-30 hours

**Action Items:**
- [ ] Implement INI/config file parser
- [ ] Implement JSON parser (using jsoncpp)
- [ ] Complete configuration validation
- [ ] Add configuration error reporting
- [ ] Test with real configuration files

**Current State:** `loadFromFile()` just checks if file exists, doesn't parse

**Target:** MVP release

---

### 4. Module File Operations
**Status:** ⚠️ **Stubbed**
**Priority:** 🔴 **CRITICAL**
**Estimated Effort:** 30-40 hours

**Action Items:**
- [ ] Implement file read operations
- [ ] Implement file write operations
- [ ] Implement file delete operations
- [ ] Implement directory listing
- [ ] Implement path validation
- [ ] Implement permission checking

**Current State:** Module class exists but all file operations are stubs

**Target:** MVP release

---

### 5. Basic Authentication
**Status:** ❌ **Not Started**
**Priority:** 🔴 **HIGH**
**Estimated Effort:** 20-30 hours

**Action Items:**
- [ ] Implement password file parsing
- [ ] Implement authentication flow
- [ ] Implement session authentication
- [ ] Add authentication error handling

**Current State:** Configuration structures exist, no implementation

**Target:** MVP release

---

### 6. Test Infrastructure & Tests
**Status:** ⚠️ **Structure Only**
**Priority:** 🔴 **HIGH**
**Estimated Effort:** 30-50 hours

**Action Items:**
- [ ] Set up test framework (Google Test or similar)
- [ ] Write unit tests for configuration
- [ ] Write unit tests for protocol (once implemented)
- [ ] Write integration tests
- [ ] Add test coverage reporting

**Current State:** Test directory exists but empty (just placeholder CMakeLists.txt)

**Target:** MVP release

---

## 🟡 High Priority (Important for Functionality)

### 7. SSL/TLS Implementation
**Status:** ⚠️ **Partial**
**Priority:** 🟡 **HIGH**
**Estimated Effort:** 40-60 hours

**Action Items:**
- [ ] Complete SSL/TLS implementation
- [ ] Implement certificate management
- [ ] Add SSL error handling
- [ ] Test SSL connections

**Current State:** Interface exists, implementation partial

---

### 8. Error Handling
**Status:** ⚠️ **Incomplete**
**Priority:** 🟡 **HIGH**
**Estimated Effort:** 20-30 hours

**Action Items:**
- [ ] Add comprehensive error handling
- [ ] Implement error reporting
- [ ] Add error recovery mechanisms
- [ ] Improve error messages

---

### 9. Documentation Accuracy
**Status:** ⚠️ **Misleading**
**Priority:** 🟡 **MEDIUM**
**Estimated Effort:** 15-20 hours

**Action Items:**
- [ ] Update README to match actual status
- [ ] Remove claims about unimplemented features
- [ ] Add "work in progress" disclaimers
- [ ] Update example configurations to reflect reality

**Current State:** Documentation describes features that don't exist

---

## 🟢 Medium Priority (Enhancements)

### 10. Logging Enhancements
**Status:** ⚠️ **Basic Only**
**Priority:** 🟢 **MEDIUM**
**Estimated Effort:** 15-20 hours

**Action Items:**
- [ ] Implement log rotation
- [ ] Add syslog integration
- [ ] Add structured logging
- [ ] Improve log formatting

---

### 11. Access Control Implementation
**Status:** ❌ **Not Started**
**Priority:** 🟢 **MEDIUM**
**Estimated Effort:** 20-30 hours

**Action Items:**
- [ ] Implement IP-based access control
- [ ] Implement network-based access control
- [ ] Add access logging

---

### 12. Rate Limiting Implementation
**Status:** ❌ **Not Started**
**Priority:** 🟢 **MEDIUM**
**Estimated Effort:** 20-30 hours

**Action Items:**
- [ ] Implement rate limiting logic
- [ ] Add connection rate limiting
- [ ] Add bandwidth rate limiting

---

## 📋 Summary

### Critical Path to MVP
**Total Estimated Effort:** 200-300 hours

1. RSync Protocol (100-150 hours)
2. File Transfer (50-75 hours)
3. Configuration Parsing (20-30 hours)
4. Module Operations (30-40 hours)
5. Basic Authentication (20-30 hours)
6. Testing (30-50 hours)

### To Match Documentation Claims
**Total Estimated Effort:** 500-800 hours

This includes all advanced features, SSL/TLS, monitoring, etc.

---

## 🎯 Recommendations

1. **Focus on MVP first** - Get basic rsync working before adding features
2. **Protocol first** - Implement rsync protocol before anything else
3. **Test as you go** - Write tests for each component as you implement
4. **Update docs** - Keep documentation aligned with reality

---

*Last Updated: December 2024*
