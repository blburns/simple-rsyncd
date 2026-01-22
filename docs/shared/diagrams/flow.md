# Simple RSync Daemon - Flow Diagrams

## RSync Session Flow

```mermaid
flowchart TD
    Start([TCP Connection]) --> Accept[Accept Connection]
    Accept --> CreateSession[Create RSync Session]
    CreateSession --> ReadGreeting[Read Client Greeting]
    ReadGreeting --> SendGreeting[Send Server Greeting]
    
    SendGreeting --> ReadCommand[Read Command]
    ReadCommand --> ParseCommand{Command Type?}
    
    ParseCommand -->|@RSYNCD| HandleRSYNCD[Handle RSYNCD]
    ParseCommand -->|Module List| HandleList[Handle Module List]
    ParseCommand -->|Module Request| HandleModule[Handle Module Request]
    ParseCommand -->|File List| HandleFileList[Handle File List]
    ParseCommand -->|File Transfer| HandleTransfer[Handle File Transfer]
    ParseCommand -->|Other| SendError[Send Error]
    
    HandleRSYNCD --> ValidateVersion{Version Compatible?}
    ValidateVersion -->|Yes| SendOK[Send OK]
    ValidateVersion -->|No| SendError
    
    HandleList --> GetModules[Get Available Modules]
    GetModules --> SendModules[Send Module List]
    
    HandleModule --> ParseModule[Parse Module Name]
    ParseModule --> LookupModule{Module Exists?}
    LookupModule -->|No| SendError
    LookupModule -->|Yes| CheckAuth[Check Authentication]
    
    CheckAuth --> AuthOK{Authenticated?}
    AuthOK -->|No| SendAuthError[Send Auth Error]
    AuthOK -->|Yes| SendModuleInfo[Send Module Info]
    
    HandleFileList --> CheckPerm{List Permission?}
    CheckPerm -->|No| SendPermError[Send Permission Error]
    CheckPerm -->|Yes| ListFiles[List Files in Module]
    ListFiles --> SendFileList[Send File List]
    
    HandleTransfer --> CheckRead{Read Permission?}
    CheckRead -->|No| SendPermError
    CheckRead -->|Yes| OpenFile[Open File]
    OpenFile --> TransferFile[Transfer File Data]
    TransferFile --> CloseFile[Close File]
    
    SendOK --> ReadCommand
    SendError --> End([End])
    SendAuthError --> End
    SendPermError --> End
    SendModules --> ReadCommand
    SendModuleInfo --> ReadCommand
    SendFileList --> ReadCommand
    CloseFile --> ReadCommand
```

