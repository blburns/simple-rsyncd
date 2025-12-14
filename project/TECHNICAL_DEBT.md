# Simple RSync Daemon - Technical Debt

**Date:** December 2024
**Current Version:** 0.2.0
**Purpose:** Track technical debt, known issues, and areas requiring improvement

---

## 🎯 Overview

This document tracks technical debt for the simple-rsyncd project. **v0.2.0 MVP is complete**, so critical blocking items have been resolved. Remaining items are enhancements for future releases.

**Total Debt Items:** 8+
**Estimated Effort:** ~100-200 hours (for enhancements, not blocking)

---

## ✅ v0.2.0 MVP - COMPLETE

All critical MVP items have been completed:
- ✅ RSync Protocol Implementation
- ✅ File Transfer Engine
- ✅ Configuration File Parsing (INI format)
- ✅ Module File Operations
- ✅ Basic Authentication
- ✅ Test Framework

---

## 🟡 Medium Priority (v0.3.0 Enhancements)

### 1. Password Hashing
**Status:** ⚠️ **Needs Implementation**
**Priority:** 🟡 **MEDIUM**
**Estimated Effort:** 10-15 hours

**Action Items:**
- [ ] Replace plain text passwords with bcrypt or argon2
- [ ] Implement password hashing on storage
- [ ] Implement password verification
- [ ] Add migration tools for existing password files
- [ ] Update password file format

**Current State:** Passwords stored in plain text (functional but insecure)

**Target:** v0.3.0 release

---

### 2. JSON Configuration Parsing
**Status:** ⚠️ **Stubbed**
**Priority:** 🟡 **MEDIUM**
**Estimated Effort:** 15-20 hours

**Action Items:**
- [ ] Implement JSON parser using jsoncpp
- [ ] Complete JSON configuration validation
- [ ] Add JSON error reporting
- [ ] Test with real JSON configuration files

**Current State:** INI format complete, JSON parser is stub

**Target:** v0.3.0 release

---

### 3. Integration Tests
**Status:** ⚠️ **Not Started**
**Priority:** 🟡 **MEDIUM**
**Estimated Effort:** 20-30 hours

**Action Items:**
- [ ] Implement end-to-end protocol tests
- [ ] Implement network integration tests
- [ ] Implement file transfer integration tests
- [ ] Add test fixtures and helpers
- [ ] Set up CI/CD test automation

**Current State:** Unit tests complete, integration tests missing

**Target:** v0.3.0 release

---

### 4. Enhanced Protocol Features
**Status:** ⚠️ **Basic Implementation Only**
**Priority:** 🟡 **MEDIUM**
**Estimated Effort:** 30-50 hours

**Action Items:**
- [ ] Implement delta sync algorithm
- [ ] Implement checksum calculation and verification
- [ ] Full rsync protocol compatibility
- [ ] Protocol optimization
- [ ] Advanced protocol features

**Current State:** Basic protocol working, advanced features missing

**Target:** v0.3.0 or v0.4.0 release
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
