#import <UIKit/UIKit.h>
#import <CoreLocation/CoreLocation.h>

#import "MWMTypes.h"

@class MWMMapSearchResult;

typedef NS_ENUM(NSUInteger, MWMZoomMode) {
  MWMZoomModeIn = 0,
  MWMZoomModeOut
};

NS_ASSUME_NONNULL_BEGIN

typedef void (^SearchInDownloaderCompletions)(NSArray<MWMMapSearchResult *> *results, BOOL finished);

NS_SWIFT_NAME(FrameworkHelper)
@interface MWMFrameworkHelper : NSObject

+ (void)processFirstLaunch:(BOOL)hasLocation;
+ (void)setVisibleViewport:(CGRect)rect scaleFactor:(CGFloat)scale;
+ (void)setTheme:(MWMTheme)theme;
+ (MWMDayTime)daytimeAtLocation:(nullable CLLocation *)location;
+ (void)createFramework;
+ (BOOL)canUseNetwork;
+ (BOOL)isNetworkConnected;
+ (BOOL)isWiFiConnected;
+ (MWMMarkGroupID)invalidCategoryId;
+ (void)zoomMap:(MWMZoomMode)mode;
+ (void)moveMap:(UIOffset)offset;
+ (void)deactivateMapSelection:(BOOL)notifyUI NS_SWIFT_NAME(deactivateMapSelection(notifyUI:));
+ (void)switchMyPositionMode;
+ (void)stopLocationFollow;
+ (NSArray<NSString *> *)obtainLastSearchQueries;
+ (void)rotateMap:(double)azimuth animated:(BOOL)isAnimated;
+ (void)updatePositionArrowOffset:(BOOL)useDefault offset:(int)offsetY;
+ (BOOL)shouldShowCrown;
+ (void)uploadUGC:(void (^)(UIBackgroundFetchResult))completionHandler;
+ (NSString *)userAccessToken;
+ (NSString *)userAgent;
+ (NSNumber *)dataVersion;
+ (void)searchInDownloader:(NSString *)query
               inputLocale:(NSString *)locale
                completion:(SearchInDownloaderCompletions)completion;

@end

NS_ASSUME_NONNULL_END
