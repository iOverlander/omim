#import "MWMNetworkPolicy+UI.h"
#import "MWMAlertViewController.h"

@implementation MWMNetworkPolicy (UI)

- (BOOL)isTempPermissionValid {
  return [self.permissionExpirationDate compare:[NSDate date]] == NSOrderedDescending;
}

- (void)askPermissionWithCompletion:(MWMBoolBlock)completion {
  MWMAlertViewController * alertController = [MWMAlertViewController activeAlertController];
  [alertController presentMobileInternetAlertWithBlock:^(MWMMobileInternetAlertResult result) {
    switch (result) {
      case MWMMobileInternetAlertResultAlways:
        completion(YES);
        break;
      case MWMMobileInternetAlertResultToday:
        completion(YES);
        break;
      case MWMMobileInternetAlertResultNotToday:
        completion(NO);
        break;
    }
  }];
}

- (void)callOnlineApi:(MWMBoolBlock)onlineCall {
  [self callOnlineApi:onlineCall forceAskPermission:NO];
}

- (void)callOnlineApi:(MWMBoolBlock)onlineCall forceAskPermission:(BOOL)askPermission {
  switch (self.connectionType) {
    case MWMConnectionTypeNone:
      onlineCall(NO);
      break;
    case MWMConnectionTypeWifi:
      onlineCall(YES);
      break;
    case MWMConnectionTypeCellular:
      switch (self.permission) {
        case MWMNetworkPolicyPermissionAsk:
          [self askPermissionWithCompletion:onlineCall];
          break;
        case MWMNetworkPolicyPermissionAlways:
          onlineCall(YES);
          break;
        case MWMNetworkPolicyPermissionNever:
          if (askPermission) {
            [self askPermissionWithCompletion:onlineCall];
          } else {
            onlineCall(NO);
          }
          break;
        case MWMNetworkPolicyPermissionToday:
          if (self.isTempPermissionValid) {
            onlineCall(YES);
          } else if (askPermission) {
            [self askPermissionWithCompletion:onlineCall];
          } else {
            onlineCall(NO);
          }
          break;
        case MWMNetworkPolicyPermissionNotToday:
          if (!self.isTempPermissionValid || askPermission) {
            [self askPermissionWithCompletion:onlineCall];
          } else {
            onlineCall(NO);
          }
          break;
      }
      break;
  }
}

@end
