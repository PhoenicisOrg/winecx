#import <AppKit/AppKit.h>
#import <Quartz/Quartz.h>

#import <Carbon/Carbon.h>

int do_print_file(char *psFileName) {
    __block int ret = 0;

    void (^block)(void) = ^{
        NSString *nsPsFileName = [NSString stringWithCString:psFileName encoding:NSASCIIStringEncoding];
        NSPrintInfo *printInfo = [NSPrintInfo sharedPrintInfo];

        PDFDocument *pdfDocument = [[PDFDocument alloc] initWithURL:[NSURL URLWithString: nsPsFileName]];

        [printInfo setLeftMargin:0.0];
        [printInfo setRightMargin:0.0];
        [printInfo setTopMargin:0.0];
        [printInfo setBottomMargin:0.0];

        NSImage *pic =  [[NSImage alloc] initWithContentsOfFile: nsPsFileName];

        if (pic.size.width < pic.size.height) {
            [printInfo setPaperSize:NSMakeSize(595, 842)];
        } else {
            [printInfo setPaperSize:NSMakeSize(842, 595)];
        }

        [printInfo setHorizontalPagination:NSFitPagination];
        [printInfo setVerticalPagination:NSFitPagination];


        NSRect picRect = NSRectFromCGRect(CGRectMake(0, 0, pic.size.width, pic.size.height));
        NSImageView *imageView = [[NSImageView alloc] initWithFrame:picRect];
        [imageView setImage:pic];

        /* NSWindow* window  = [[[NSWindow alloc] initWithContentRect:picRect
                                                         styleMask:NSWindowStyleMaskClosable
                                                           backing:NSBackingStoreBuffered
                                                             defer:NO] autorelease];
        [window makeKeyAndOrderFront:NSApp];
        [window.contentView addSubview:imageView];
        */

        NSPrintOperation * picPrint = [NSPrintOperation printOperationWithView:imageView printInfo:printInfo];
        //PDFPrintScalingMode scale = kPDFPrintPageScaleDownToFit;
        //NSPrintOperation *picPrint = [pdfDocument printOperationForPrintInfo: printInfo scalingMode: scale autoRotate: YES];

        [picPrint setCanSpawnSeparateThread:YES];
        [picPrint runOperation];
    };

    if ([NSThread isMainThread]) block();
    else dispatch_sync(dispatch_get_main_queue(), block);

    return ret;
}
