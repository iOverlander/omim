#import "MPNativeAd+MWM.h"
#import "SwiftBridge.h"

@interface MPNativeAd ()

@property(nonatomic) MPNativeView * associatedView;
@property(nonatomic) BOOL hasAttachedToView;
@property(nonatomic, readonly) id<MPNativeAdAdapter> adAdapter;

- (void)willAttachToView:(UIView *)view withAdContentViews:(NSArray *)adContentViews;
- (void)adViewTapped;
- (void)nativeViewWillMoveToSuperview:(UIView *)superview;
- (UIViewController *)viewControllerForPresentingModalView;

@end

@implementation MPNativeAd (MWM)

- (void)setAdView:(UIView *)view actionButtons:(NSArray<UIButton *> *)buttons
{
  self.associatedView = (MPNativeView *)view;
  ((MWMAdBanner *)view).mpNativeAd = self;
  if (!self.hasAttachedToView) {
    id<MPNativeAdAdapter> adapter = self.adAdapter;
    if ([adapter isKindOfClass:[FacebookNativeAdAdapter class]])
    {
      FacebookNativeAdAdapter *fbAdapter = (FacebookNativeAdAdapter *)adapter;
      [fbAdapter.fbNativeAd registerViewForInteraction:self.associatedView
                                    withViewController:[self viewControllerForPresentingModalView]
                                    withClickableViews:buttons];
    }
    else
    {
      [self willAttachToView:self.associatedView withAdContentViews:self.associatedView.subviews];
      for (UIButton * button in buttons)
      {
        [button addTarget:self
                      action:@selector(adViewTapped)
            forControlEvents:UIControlEventTouchUpInside];
      }
    }
    self.hasAttachedToView = YES;
  }
}

- (void)unregister
{
  id<MPNativeAdAdapter> adapter = self.adAdapter;
  self.delegate = nil;
  [self nativeViewWillMoveToSuperview:nil];
  self.associatedView = nil;
  self.hasAttachedToView = NO;
  if ([adapter isKindOfClass:[FacebookNativeAdAdapter class]])
  {
    FacebookNativeAdAdapter *fbAdapter = (FacebookNativeAdAdapter *)adapter;
    [fbAdapter.fbNativeAd unregisterView];
  }
}

@end
