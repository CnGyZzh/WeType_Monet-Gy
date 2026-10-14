package com.gy.wetype.hook;

import android.app.Application;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.inputmethodservice.InputMethodService;
import android.view.View;
import android.util.TypedValue;
import android.view.Window;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
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
    private volatile Map<Integer,Integer> transparentResources=new HashMap<>();
    private volatile int toolbarId=-1;
    private volatile boolean lastIndependent=false;
    private volatile boolean glassEnabled=true, hookReady=false;
    private volatile int keyOpacity=50;
    private static final String CONTROL="com.gy.gyglass.lsp.CONTROL";
    private static final String REPLY="com.gy.gyglass.lsp.REPLY";

    @Override public void handleLoadPackage(XC_LoadPackage.LoadPackageParam param) {
        if (!TARGET.equals(param.packageName)) return;
        XposedHelpers.findAndHookMethod(Application.class, "attach", Context.class, new XC_MethodHook() {
            @Override protected void afterHookedMethod(MethodHookParam hook) {
                Context ctx = (Context) hook.args[0];
                if (!TARGET.equals(ctx.getPackageName())) return;
                appContext = ctx;
                registerControls(ctx);
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
            Class<?> colors=XposedHelpers.findClassIfExists("com.tencent.wetype.plugin.hld.p",loader);
            Class<?> ids=XposedHelpers.findClassIfExists("com.tencent.wetype.plugin.hld.s",loader);
            HashMap<Integer,Integer> clear=new HashMap<>();
            for(String n:new String[]{"ime_skin_keyboard_start_color","ime_skin_keyboard_end_color",
                "ime_skin_dark_keyboard_start_color","ime_skin_dark_keyboard_end_color",
                "ime_skin_candidate_start_color","ime_skin_candidate_end_color",
                "ime_skin_dark_candidate_start_color","ime_skin_dark_candidate_end_color"}){
                if(colors!=null)add(clear,colors,n,1);
                if(ids!=null)add(clear,ids,n,1);
            }
            transparentResources=clear;
            if(ids!=null)try{toolbarId=ids.getField("custom_toolbar_item_container_view").getInt(null);}
                catch(Throwable ignored){}
            readAppearance();

            XposedBridge.hookAllMethods(Resources.Theme.class,"resolveAttribute",new XC_MethodHook() {
                @Override protected void afterHookedMethod(MethodHookParam hook) {
                    if (!glassEnabled || !Boolean.TRUE.equals(hook.getResult())) return;
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
            addKeyboardBackgroundHooks();
            addToolbarHook();
            addWindowHook(loader);
            hookReady=true;
            writeStatus("active");
            XposedBridge.log(TAG+"HOOK_ACTIVE attributes="+roles.size()+" transparentColors="+clear.size());
        } catch(Throwable e) { XposedBridge.log(TAG+"HOOK_FAILED "+e); }
    }

    private void addWindowHook(ClassLoader loader) {
        XC_MethodHook apply = new XC_MethodHook() {
            @Override protected void afterHookedMethod(MethodHookParam hook) {
                if (!glassEnabled || !(hook.thisObject instanceof InputMethodService)) return;
                try {
                    readAppearance();
                    InputMethodService ime=(InputMethodService)hook.thisObject;
                    if(ime.getWindow()==null || ime.getWindow().getWindow()==null) return;
                    Window window=ime.getWindow().getWindow();
                    // No full-window blur: it blurs WeChat content outside the keyboard.
                    window.setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
                    writeStatus("visible");
                } catch(Throwable e) { XposedBridge.log(TAG+"WINDOW_FAILED "+e); }
            }
        };
        try { XposedBridge.hookAllMethods(InputMethodService.class,"onWindowShown",apply); }
        catch(Throwable e) { XposedBridge.log(TAG+"WINDOW_STAGE_UNAVAILABLE "+e); }
        try { XposedBridge.hookAllMethods(InputMethodService.class,"onStartInputView",apply); }
        catch(Throwable e) { XposedBridge.log(TAG+"WINDOW_STAGE_UNAVAILABLE "+e); }
        try {
            Class<?> wxService=XposedHelpers.findClassIfExists("com.tencent.wetype.plugin.hld.WxHldService",loader);
            if (wxService!=null) {
                XposedBridge.hookAllMethods(wxService,"onStartInputView",apply);
                try { XposedBridge.hookAllMethods(wxService,"onCreateInputView",new XC_MethodHook(){
                    @Override protected void afterHookedMethod(MethodHookParam p){
                        if(p.getResult() instanceof View)
                            ((View)p.getResult()).setBackgroundColor(Color.TRANSPARENT);
                    }
                }); } catch(Throwable ignored){}
            }
        } catch(Throwable e) { XposedBridge.log(TAG+"HOST_STAGE_UNAVAILABLE "+e); }
    }

    private void addKeyboardBackgroundHooks(){
        if(transparentResources.isEmpty())return;
        XposedBridge.hookAllMethods(Resources.class,"getColor",new XC_MethodHook(){
            @Override protected void afterHookedMethod(MethodHookParam p){
                if(glassEnabled && p.args.length>0 && p.args[0] instanceof Integer &&
                    transparentResources.containsKey((Integer)p.args[0]))
                    p.setResult(Color.TRANSPARENT);
            }
        });
        XposedBridge.hookAllMethods(Resources.class,"getColorStateList",new XC_MethodHook(){
            @Override protected void afterHookedMethod(MethodHookParam p){
                if(p.args.length>0 && p.args[0] instanceof Integer &&
                    transparentResources.containsKey((Integer)p.args[0]))
                    p.setResult(ColorStateList.valueOf(Color.TRANSPARENT));
            }
        });
    }
    private void addToolbarHook(){
        if(toolbarId<0)return;
        XC_MethodHook adjust=new XC_MethodHook(){
            @Override protected void beforeHookedMethod(MethodHookParam p){
                if(!glassEnabled || !(p.thisObject instanceof View) || ((View)p.thisObject).getId()!=toolbarId ||
                   p.args.length<1 || !(p.args[0] instanceof Drawable))return;
                Drawable d=((Drawable)p.args[0]).mutate(); d.setAlpha(72); p.args[0]=d;
            }
        };
        XposedBridge.hookAllMethods(View.class,"setBackground",adjust);
        XposedBridge.hookAllMethods(View.class,"setBackgroundDrawable",adjust);
    }
    private void registerControls(Context ctx) {
        try {
            IntentFilter f=new IntentFilter(CONTROL);
            ctx.registerReceiver(new BroadcastReceiver(){
                @Override public void onReceive(Context c,Intent intent) {
                    if(!CONTROL.equals(intent.getAction()))return;
                    String cmd=intent.getStringExtra("cmd");
                    if("apply".equals(cmd)) {
                        try {
                            ctx.getSharedPreferences("gyglass_custom",Context.MODE_PRIVATE).edit()
                                .putBoolean("enabled",intent.getBooleanExtra("enabled",true))
                                .putBoolean("independent",intent.getBooleanExtra("independent",false))
                                .putInt("opacity",Math.max(0,Math.min(100,intent.getIntExtra("opacity",50))))
                                .apply();
                            readAppearance();
                            writeStatus("config_applied");
                        } catch(Throwable t){XposedBridge.log(TAG+"CONFIG_FAILED "+t);}
                    }
                    Intent reply=new Intent(REPLY);
                    reply.setPackage("com.gy.gyglass.lsp");
                    reply.putExtra("ready",hookReady);
                    reply.putExtra("applied","apply".equals(cmd));
                    reply.putExtra("process",android.os.Process.myPid());
                    try{ctx.sendBroadcast(reply);}catch(Throwable ignored){}
                }
            },f,Context.RECEIVER_EXPORTED);
            XposedBridge.log(TAG+"PROCESS_ATTACHED / receiver registered");
        }catch(Throwable t){XposedBridge.log(TAG+"CONTROL_RECEIVER_FAILED "+t);}
    }
    private void writeStatus(String event){
        Context ctx=appContext;if(ctx==null)return;
        try {
            File f=new File(ctx.getFilesDir(),"gyglass_runtime.txt");
            String t="HOOK=active\nMODE="+(lastIndependent?"on":"off")+
              "\nEVENT="+event+"\nAT="+System.currentTimeMillis()/1000+"\n";
            try(FileOutputStream o=new FileOutputStream(f,false)){
                o.write(t.getBytes(StandardCharsets.UTF_8));
            }
        }catch(Throwable ignored){}
    }
    private void readAppearance() {
        Context ctx=appContext;
        if(ctx==null) return;
        SharedPreferences sp=ctx.getSharedPreferences("gyglass_custom",Context.MODE_PRIVATE);
        glassEnabled=sp.getBoolean("enabled",true);
        boolean independent=sp.getBoolean("independent",readPreset(ctx));
        keyOpacity=Math.max(0,Math.min(100,sp.getInt("opacity",50)));
        lastIndependent=independent;
        boolean dark=(ctx.getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK)==Configuration.UI_MODE_NIGHT_YES;
        int neutral=monet(ctx,dark?"system_neutral1_700":"system_neutral1_100",dark?0xFF474250:0xFFCCC8D9);
        panelColor=alpha(neutral,dark?0x20:0x28);
        int adjustable=(int)Math.round(keyOpacity*2.55);
        normalFill=alpha(neutral,independent?adjustable:0);
        functionFill=alpha(neutral,independent?Math.min(255,adjustable+12):Math.min(255,adjustable/2));
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
