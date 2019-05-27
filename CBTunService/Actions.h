//
//  Actions.h
//  SMJobBless
//
//  Created by Ludovic Delaveau on 8/5/12.
//
//

#import <Foundation/Foundation.h>

@protocol TunHandler <NSObject>

- (void)openTun:(void (^)(NSFileHandle *tun, NSError *error))reply;
- (void)closeTun;
- (void) readData:(void (^)(NSData * data, NSError *error))reply;
- (void) writeData:(NSData*)data;

@end
