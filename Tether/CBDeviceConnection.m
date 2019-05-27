//
//  CBDeviceConnection.m
//  Tether
//
//  Created by Christopher Ballinger on 11/10/13.
//  Copyright (c) 2013 Christopher Ballinger. All rights reserved.
//

#import "CBDeviceConnection.h"

#define DEVICE_CONNECTION_RECEIVE_TAG 100
#define LOCAL_SOCKET_READ_TAG 200
#define LOCAL_SOCKET_WRITE_TAG 201

@implementation CBDeviceConnection

- (void) dealloc {
    [self disconnect];
}

- (id) initWithDeviceConnection:(USBMuxDeviceConnection*)connection socket:(GCDAsyncSocket*)socket {
    if (self = [super init]) {
        _deviceConnection = connection;
        _deviceConnection.delegate = self;
        _socket = socket;
        _socket.delegate = self;
        [_socket readDataWithTimeout:-1 tag:LOCAL_SOCKET_READ_TAG];
        self.delegateQueue = dispatch_get_main_queue();
    }
    return self;
}

- (void) socket:(GCDAsyncSocket *)sock didReadData:(NSData *)data withTag:(long)tag {
    //NSLog(@"local socket %@ did read %ld data: %@", sock, tag, data);
    [_deviceConnection writeData:data tag:tag];
    [_deviceConnection readDataWithTimeout:-1 tag:tag];
    [sock readDataWithTimeout:-1 tag:LOCAL_SOCKET_READ_TAG];
}

- (void) socket:(GCDAsyncSocket *)sock didWriteDataWithTag:(long)tag {
    //NSLog(@"local socket %@ did write data with tag: %ld", sock, tag);
    [_socket readDataWithTimeout:-1 tag:LOCAL_SOCKET_WRITE_TAG];
}

- (void) connection:(USBMuxDeviceConnection*)connection didReadData:(NSData *)data tag:(long)tag {
    //NSLog(@"connection %@ did receive data %ld: %@", connection, tag, data);
    [_socket writeData:data withTimeout:-1 tag:LOCAL_SOCKET_WRITE_TAG];
    [connection readDataWithTimeout:-1 tag:tag];
    if (_delegate && [_delegate respondsToSelector:@selector(connection:didReadData:)]) {
        [_delegate connection:self didReadData:data];
    }
}

- (void) connection:(USBMuxDeviceConnection*)connection didWriteDataToLength:(NSUInteger)length tag:(long)tag {
    //NSLog(@"connection %@ did write data %ld to length: %lu", connection, tag, (unsigned long)length);
    if (_delegate && [_delegate respondsToSelector:@selector(connection:didWriteDataToLength:)]) {
        [_delegate connection:self didWriteDataToLength:length];
    }
}


- (void) disconnect {
    if (self.deviceConnection) {
        [self.deviceConnection disconnect];
        self.deviceConnection.delegate = nil;
        self.deviceConnection = nil;
    }
    if (self.socket) {
        [self.socket disconnect];
        self.socket.delegate = nil;
        self.socket = nil;
    }
}

@end
