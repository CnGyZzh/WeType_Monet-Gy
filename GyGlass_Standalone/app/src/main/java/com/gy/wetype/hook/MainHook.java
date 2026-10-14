package com.gy.wetype.hook;

import android.app.Application;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.inputmethodservice.InputMethodService;
import android.util.TypedValue;
import android.view.Window;

import java.io.File;
import java.io.FileInputStream;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;

import de.robv.android.xposed.IXposedHookLoadPackage;
import de.robv.android.xposed.XC_MethodHook;
import de.robv.android.xposed.XposedBridge;
import de.robv.android.xposed.XposedHelpers;
import de.robv.android.xposed.callbacks.XC_LoadPackage;

/** Gy independent glass keyboard hook. No WeTyper binary, licensing or launcher. */
public final class MainHook implements IXposedHookLoadPackage {
    private static final String TARGET = "com.tencent.wetype";
    private static final String TAG = "GyGlass: ";
    private static final int FILL_NORMAL = 1, FILL_FUNCTION = 2, SHADOW = 3, BORDER = 4;
    private final AtomicBoolean installed = new AtomicBoolean(false);
    private volatile Map<Integer,Integer> targetAttributes = new HashMap<>();
    private volatile int normalFill = Color.TRANSPARENT;
    private volatile int functionFill = 0x32BEBACC;
    private volatile int borderColor = 0x20FFFFFF;
    private volatile int panelColor = 0x50BEBACC;
    private volatile Context appContext;

    @Override public void handleLoadPackage(XC_LoadPackage.LoadPackageParam param) {
        if (!TARGET.equals(param.packageName)) return;
        XposedHelpers.findAndHookMethod(Application.class, "attach", Context.class, new XC_MethodHook() {
            @Override protected void afterHookedMethod(MethodHookParam hook) {
                Context ctx = (Context) hook.args[0];
                if (!TARGET.equals(ctx.getPackageName())) return;
                appContext = ctx;
                if (installed.compareAndSet(false, true)) install(ctx.getClassLoader());
            }
        });
    }

    private static void add(Map<Integer,Integer> values, Class<?> attrs, String field, int role) {
        try { values.put(attrs.getField(field).getInt(null), role); }
        catch (ReflectiveOperationException ignored) { }
    }

    private void install(ClassLoader loader) {
        try {
            Class<?> attrs = XposedHelpers.findClassIfExists("com.tencent.wetype.plugin.hld.o",loader);
            if (attrs == null) attrs = XposedHelpers.findClassIfExists("com.tencent.wetype.plugin.hld.s",loader);
            if (attrs == null) { XposedBridge.log(TAG+"ATTR_CLASS_MISSING"); return; }
            HashMap<Integer,Integer> roles = new HashMap<>();
            for(String k : new String[]{"ime_color_btn_white_bg","ime_key_white_color"}) add(roles,attrs,k,FILL_NORMAL);
            for(String k : new String[]{"ime_color_btn_grey_bg","ime_key_grey_color"}) add(roles,attrs,k,FILL_FUNCTION);
            for(String k : new String[]{"ime_color_btn_white_shadow","ime_color_btn_grey_shadow","ime_key_white_shadow_color","ime_key_grey_shadow_color","UN_BW_0_Alpha_0_5"}) add(roles,attrs,k,SHADOW);
            for(String k : new String[]{"ime_color_btn_white_border","ime_color_btn_grey_border","ime_key_white_border_color","ime_key_grey_border_color"}) add(roles,attrs,k,BORDER);
            if (roles.isEmpty()) { XposedBridge.log(TAG+"ATTR_NOT_FOUND"); return; }
            targetAttributes = roles;
            readAppearance();

            XposedBridge.hookAllMethods(Resources.Theme.class,"resolveAttribute",new XC_MethodHook() {
                @Override protected void afterHookedMethod(MethodHookParam hook) {
                    if (!Boolean.TRUE.equals(hook.getResult())) return;
                    if (!(hook.args[0] instanceof Integer) || !(hook.args[1] instanceof TypedValue)) return;
                    Integer role = targetAttributes.get((Integer)hook.args[0]);
                    if (role == null) return;
                    int newColor;
                    if (role == FILL_NORMAL) newColor=normalFill;
                    else if (role == FILL_FUNCTION) newColor=functionFill;
                    else if (role == SHADOW) newColor=Color.TRANSPARENT;
                    else if (role == BORDER) newColor=borderColor;
                    else return;
                    TypedValue result=(TypedValue)hook.args[1];
                    result.type=TypedValue.TYPE_INT_COLOR_ARGB8;
                    result.data=newColor;
                    result.resourceId=0;
                }
            });
            addWindowHook(loader);
            XposedBridge.log(TAG+"HOOK_ACTIVE attributes="+roles.size());
        } catch(Throwable e) { XposedBridge.log(TAG+"HOOK_FAILED "+e); }
    }

    private void addWindowHook(ClassLoader loader) {
        XC_MethodHook apply = new XC_MethodHook() {
            @Override protected void afterHookedMethod(MethodHookParam hook) {
                if (!(hook.thisObject instanceof InputMethodService)) return;
                try {
                    readAppearance();
                    InputMethodService ime=(InputMethodService)hook.thisObject;
                    if(ime.getWindow()==null || ime.getWindow().getWindow()==null) return;
                    Window window=ime.getWindow().getWindow();
                    window.setBackgroundDrawable(new ColorDrawable(panelColor));
                    try { window.setBackgroundBlurRadius(20); } catch(Throwable ignored) {}
                } catch(Throwable e) { XposedBridge.log(TAG+"WINDOW_FAILED "+e); }
            }
        };
        try { XposedBridge.hookAllMethods(InputMethodService.class,"onWindowShown",apply); }
        catch(Throwable e) { XposedBridge.log(TAG+"WINDOW_STAGE_UNAVAILABLE "+e); }
        try {
            Class<?> wxService=XposedHelpers.findClassIfExists("com.tencent.wetype.plugin.hld.WxHldService",loader);
            if (wxService!=null) XposedBridge.hookAllMethods(wxService,"onStartInputView",apply);
        } catch(Throwable e) { XposedBridge.log(TAG+"HOST_STAGE_UNAVAILABLE "+e); }
    }

    private void readAppearance() {
        Context ctx=appContext;
        if(ctx==null) return;
        boolean independent = readPreset(ctx);
        boolean dark=(ctx.getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK)==Configuration.UI_MODE_NIGHT_YES;
        int neutral=monet(ctx,dark?"system_neutral1_700":"system_neutral1_100",dark?0xFF474250:0xFFCCC8D9);
        panelColor=alpha(neutral,dark?0x39:0x50);
        normalFill=alpha(neutral,independent?(dark?0x68:0x76):0);
        functionFill=alpha(neutral,independent?0x55:0x32);
        borderColor=independent?0x25FFFFFF:0x08FFFFFF;
    }

    private int monet(Context ctx,String name,int fallback) {
        try {
            Resources resources=ctx.getResources();
            int id=resources.getIdentifier(name,"color","android");
            return id==0?fallback:resources.getColor(id,ctx.getTheme());
        } catch(Throwable ignored) { return fallback; }
    }
    private static int alpha(int color,int amount) { return (color & 0x00FFFFFF)|((amount & 0xFF)<<24); }

    private boolean readPreset(Context ctx) {
        File file=new File(ctx.getApplicationInfo().dataDir,"shared_prefs/gy_glass.xml");
        if(!file.isFile()) return false;
        try(FileInputStream in=new FileInputStream(file)){
            byte[] bytes=new byte[512];
            int n=in.read(bytes);
            return n>0 && new String(bytes,0,n, StandardCharsets.UTF_8).contains("name=\"key_background\" value=\"true\"");
        } catch(Throwable ignored) { return false; }
    }
}
