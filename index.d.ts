import { EventEmitter } from "events";

export class WickrIOAddon extends EventEmitter {
  constructor(debugOn?: boolean);

  /** Initialize the bot client. Uses SQS if all three SQS queue params are provided, otherwise ZMQ. */
  clientInit(
    username: string,
    sqsRegion?: string,
    sqsMessageQueue?: string,
    sqsRequestQueue?: string,
    sqsResponseQueue?: string
  ): string;

  /** Mark the client as closed. */
  closeClient(): void;

  /** Ping the engine to check connectivity. */
  isConnected(delay?: number): Promise<boolean>;

  /** Get the current client state from the engine. */
  getClientState(): Promise<string>;

  // ---------------------------------------------------------------------------
  // Async message receipt
  // ---------------------------------------------------------------------------

  /** Start receiving messages asynchronously via the provided callback. */
  cmdStartAsyncRecvMessages(
    callback: (message: string) => void
  ): Promise<string>;

  /** Stop receiving messages asynchronously. */
  cmdStopAsyncRecvMessages(): Promise<string>;

  // ---------------------------------------------------------------------------
  // Streaming
  // ---------------------------------------------------------------------------

  /** Disable message streaming. */
  cmdSetStreamingOff(): Promise<string>;

  /** Enable file-based message streaming. */
  cmdSetFileStreaming(
    dest: string,
    baseName: string,
    maxSize: number,
    attachmentLoc: string
  ): Promise<string>;

  // ---------------------------------------------------------------------------
  // Events
  // ---------------------------------------------------------------------------

  /** Post an event to the engine. */
  cmdPostEvent(event: string): Promise<string>;

  // ---------------------------------------------------------------------------
  // URL callbacks
  // ---------------------------------------------------------------------------

  /** Set the message URL callback. */
  cmdSetMsgCallback(callback: string): Promise<string>;

  /** Get the current message URL callback. */
  cmdGetMsgCallback(): Promise<string>;

  /** Delete the message URL callback. */
  cmdDeleteMsgCallback(): Promise<string>;

  /** Set the event URL callback. */
  cmdSetEventCallback(callback: string): Promise<string>;

  /** Get the current event URL callback. */
  cmdGetEventCallback(): Promise<string>;

  /** Delete the event URL callback. */
  cmdDeleteEventCallback(): Promise<string>;

  // ---------------------------------------------------------------------------
  // Statistics
  // ---------------------------------------------------------------------------

  /** Get engine statistics. */
  cmdGetStatistics(): Promise<string>;

  /** Clear engine statistics. */
  cmdClearStatistics(): Promise<string>;

  // ---------------------------------------------------------------------------
  // Rooms
  // ---------------------------------------------------------------------------

  /** Get all rooms. */
  cmdGetRooms(): Promise<string>;

  /** Create a new room. */
  cmdAddRoom(
    members: string[],
    moderators: string[],
    title?: string,
    description?: string,
    ttl?: string,
    bor?: string
  ): Promise<string>;

  /** Modify an existing room. */
  cmdModifyRoom(
    vgroupid: string,
    members: string[],
    moderators: string[],
    title?: string,
    description?: string,
    ttl?: string,
    bor?: string
  ): Promise<string>;

  /** Get room details by vGroupID. */
  cmdGetRoom(vGroupID: string): Promise<string>;

  /** Leave a room. */
  cmdLeaveRoom(vGroupID: string): Promise<string>;

  /** Delete a room. */
  cmdDeleteRoom(vGroupID: string): Promise<string>;

  // ---------------------------------------------------------------------------
  // Group conversations
  // ---------------------------------------------------------------------------

  /** Create a new group conversation. */
  cmdAddGroupConvo(
    members: string[],
    ttl?: string,
    bor?: string
  ): Promise<string>;

  /** Delete a group conversation. */
  cmdDeleteGroupConvo(vGroupID: string): Promise<string>;

  /** Get group conversation details by vGroupID. */
  cmdGetGroupConvo(vGroupID: string): Promise<string>;

  /** Get all group conversations. */
  cmdGetGroupConvos(): Promise<string>;

  // ---------------------------------------------------------------------------
  // Received messages
  // ---------------------------------------------------------------------------

  /** Get received messages from the engine. */
  cmdGetReceivedMessage(): Promise<string>;

  // ---------------------------------------------------------------------------
  // 1-to-1 messaging
  // ---------------------------------------------------------------------------

  /** Send a 1-to-1 text message to one or more users. */
  cmdSend1to1Message(
    users: string[],
    message: string,
    ttl?: string,
    bor?: string,
    messageID?: string,
    flags?: string[],
    messageMetaData?: string,
    isLowPriority?: boolean
  ): Promise<string>;

  /** Send a 1-to-1 attachment to one or more users. */
  cmdSend1to1Attachment(
    users: string[],
    attachment: string,
    displayName?: string,
    ttl?: string,
    bor?: string,
    messageMetaData?: string,
    deleteWhenSent?: boolean,
    isLowPriority?: boolean
  ): Promise<string>;

  // ---------------------------------------------------------------------------
  // Username-file based messaging
  // ---------------------------------------------------------------------------

  /** Send a text message using a username file. */
  cmdSendMessageUserNameFile(
    userNameFile: string,
    message: string,
    ttl?: string,
    bor?: string,
    messageID?: string,
    flags?: string[],
    messageMetaData?: string,
    isLowPriority?: boolean
  ): Promise<string>;

  /** Send a text message using a user hash file. */
  cmdSendMessageUserHashFile(
    userHashFile: string,
    message: string,
    ttl?: string,
    bor?: string,
    messageID?: string,
    flags?: string[],
    messageMetaData?: string,
    isLowPriority?: boolean
  ): Promise<string>;

  /** Send an attachment using a username file. */
  cmdSendAttachmentUserNameFile(
    userNameFile: string,
    attachment: string,
    displayName?: string,
    ttl?: string,
    bor?: string,
    messageID?: string,
    messageMetaData?: string,
    message?: string,
    deleteWhenSent?: boolean
  ): Promise<string>;

  /** Send an attachment using a user hash file. */
  cmdSendAttachmentUserHashFile(
    userIDFile: string,
    attachment: string,
    displayName?: string,
    ttl?: string,
    bor?: string,
    messageID?: string,
    messageMetaData?: string,
    message?: string,
    deleteWhenSent?: boolean
  ): Promise<string>;

  /** Send a voice memo using a username file. */
  cmdSendVoiceMemoUserNameFile(
    userNameFile: string,
    attachment: string,
    displayName: string | undefined,
    duration: string,
    ttl?: string,
    bor?: string,
    messageID?: string,
    messageMetaData?: string,
    message?: string
  ): Promise<string>;

  /** Send a voice memo using a user hash file. */
  cmdSendVoiceMemoUserHashFile(
    userIDFile: string,
    attachment: string,
    displayName: string | undefined,
    duration: string,
    ttl?: string,
    bor?: string,
    messageID?: string,
    messageMetaData?: string,
    message?: string
  ): Promise<string>;

  // ---------------------------------------------------------------------------
  // Room messaging
  // ---------------------------------------------------------------------------

  /** Send a text message to a room. */
  cmdSendRoomMessage(
    vGroupID: string,
    message: string,
    ttl?: string,
    bor?: string,
    messageID?: string,
    flags?: string[],
    messageMetaData?: string
  ): Promise<string>;

  /** Send an attachment to a room. */
  cmdSendRoomAttachment(
    vGroupID: string,
    attachment: string,
    displayName?: string,
    ttl?: string,
    bor?: string,
    messageMetaData?: string,
    deleteWhenSent?: boolean
  ): Promise<string>;

  // ---------------------------------------------------------------------------
  // Network messaging
  // ---------------------------------------------------------------------------

  /** Send a text message to the entire network. */
  cmdSendNetworkMessage(
    message: string,
    ttl?: string,
    bor?: string,
    messageID?: string,
    flags?: string[],
    messageMetaData?: string
  ): Promise<string>;

  /** Send a text message to specific security groups. */
  cmdSendSecurityGroupMessage(
    message: string,
    securityGroups: string[],
    ttl?: string,
    bor?: string,
    messageID?: string,
    flags?: string[],
    messageMetaData?: string
  ): Promise<string>;

  /** Send an attachment to the entire network. */
  cmdSendNetworkAttachment(
    attachment: string,
    displayName?: string,
    ttl?: string,
    bor?: string,
    messageID?: string,
    message?: string,
    messageMetaData?: string,
    deleteWhenSent?: boolean
  ): Promise<string>;

  /** Send an attachment to specific security groups. */
  cmdSendSecurityGroupAttachment(
    securityGroups: string[],
    attachment: string,
    displayName?: string,
    ttl?: string,
    bor?: string,
    messageID?: string,
    message?: string,
    messageMetaData?: string,
    deleteWhenSent?: boolean
  ): Promise<string>;

  /** Send a voice memo to the entire network. */
  cmdSendNetworkVoiceMemo(
    attachment: string,
    displayName: string | undefined,
    duration: string,
    ttl?: string,
    bor?: string,
    messageID?: string,
    message?: string,
    messageMetaData?: string
  ): Promise<string>;

  /** Send a voice memo to specific security groups. */
  cmdSendSecurityGroupVoiceMemo(
    securityGroups: string[],
    attachment: string,
    displayName: string | undefined,
    duration: string,
    ttl?: string,
    bor?: string,
    messageID?: string,
    message?: string,
    messageMetaData?: string
  ): Promise<string>;

  // ---------------------------------------------------------------------------
  // Encryption
  // ---------------------------------------------------------------------------

  /** Encrypt a string value using the engine. */
  cmdEncryptString(value: string): Promise<string>;

  /** Decrypt a string value using the engine. */
  cmdDecryptString(value: string): Promise<string>;

  // ---------------------------------------------------------------------------
  // Key-value store
  // ---------------------------------------------------------------------------

  /** Add a key-value pair. */
  cmdAddKeyValue(key: string, value: string): Promise<string>;

  /** Get the value for a key. */
  cmdGetKeyValue(key: string): Promise<string>;

  /** Delete a key-value pair. */
  cmdDeleteKeyValue(key: string): Promise<string>;

  /** Clear all key-value pairs. */
  cmdClearAllKeyValues(): Promise<string>;

  // ---------------------------------------------------------------------------
  // Controls
  // ---------------------------------------------------------------------------

  /** Set a control key to the given value. */
  cmdSetControl(controlKey: string, controlValue: string): Promise<string>;

  // ---------------------------------------------------------------------------
  // Network rooms
  // ---------------------------------------------------------------------------

  /** Get rooms for a network. */
  cmdGetNetworkRooms(network: string): Promise<string>;

  /** Add a network room. */
  cmdAddNetworkRoom(
    network: string,
    securityGroups?: string[],
    title?: string,
    description?: string,
    ttl?: string,
    bor?: string
  ): Promise<string>;

  // ---------------------------------------------------------------------------
  // Directory & security groups
  // ---------------------------------------------------------------------------

  /** Get the user directory, optionally paginated. */
  cmdGetDirectory(page?: number | string, size?: number | string): Promise<string>;

  /** Get security groups, optionally paginated. */
  cmdGetSecurityGroups(page?: number | string, size?: number | string): Promise<string>;

  /** Get the directory for a specific security group, optionally paginated. */
  cmdGetSecurityGroupDirectory(
    securityGroup: string,
    page?: number | string,
    size?: number | string
  ): Promise<string>;

  // ---------------------------------------------------------------------------
  // Message status & message IDs
  // ---------------------------------------------------------------------------

  /** Get the status of a message by its ID. */
  cmdGetMessageStatus(
    messageID: string,
    type?: "full" | "summary",
    page?: number | string,
    size?: number | string,
    filter?: string,
    usersFilter?: string
  ): Promise<string>;

  /** Set the status of a message for a specific user. */
  cmdSetMessageStatus(
    messageID: string,
    user: string,
    status: string,
    statusMessage?: string
  ): Promise<string>;

  /** Add a message ID entry. */
  cmdAddMessageID(
    messageID: string,
    sender: string,
    target: string,
    dateSent: string,
    message: string
  ): Promise<string>;

  /** Delete a message ID entry. */
  cmdDeleteMessageID(messageID: string): Promise<string>;

  /** Get a message ID entry. */
  cmdGetMessageIDEntry(messageID: string): Promise<string>;

  /** Get the message ID table, optionally filtered by sender. */
  cmdGetMessageIDTable(
    page: number | string,
    size: number | string,
    sender?: string
  ): Promise<string>;

  /** Cancel a message by its ID. */
  cmdCancelMessageID(messageID: string): Promise<string>;

  // ---------------------------------------------------------------------------
  // Delete & recall messages
  // ---------------------------------------------------------------------------

  /** Delete a message in a conversation. */
  cmdSendDeleteMessage(vGroupID: string, msgID: string): Promise<string>;

  /** Recall a message in a conversation. */
  cmdSendRecallMessage(vGroupID: string, msgID: string): Promise<string>;

  // ---------------------------------------------------------------------------
  // Verification
  // ---------------------------------------------------------------------------

  /** Get the verification list. Pass `"all"` for all users. */
  cmdGetVerificationList(mode?: string): Promise<string>;

  /** Verify specific users. */
  cmdVerifyUsers(users: string[]): Promise<string>;

  /** Verify all users. */
  cmdVerifyAll(): Promise<string>;

  /** Set the verification mode. */
  cmdSetVerificationMode(mode: string): Promise<string>;

  // ---------------------------------------------------------------------------
  // User / server / client info
  // ---------------------------------------------------------------------------

  /** Get information about specific users. */
  cmdGetUserInfo(users: string[]): Promise<string>;

  /** Get server information. */
  cmdGetServerInfo(): Promise<string>;

  /** Get client information (returns the client name). */
  cmdGetClientInfo(): Promise<{ name: string }>;

  // ---------------------------------------------------------------------------
  // Avatar
  // ---------------------------------------------------------------------------

  /** Set the bot's avatar image. */
  cmdSetAvatar(fileName: string): Promise<string>;

  // ---------------------------------------------------------------------------
  // Bots
  // ---------------------------------------------------------------------------

  /** Get the list of bots for a network. */
  cmdGetBotsList(
    networkID: string,
    tokenCredentials: string
  ): Promise<string>;

  // ---------------------------------------------------------------------------
  // Transmit queue
  // ---------------------------------------------------------------------------

  /** Get transmit queue information. */
  cmdGetTransmitQueueInfo(): Promise<string>;
}
