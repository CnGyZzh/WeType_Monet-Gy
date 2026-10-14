package com.gy.wetype.hook;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.View;
import android.view.WindowInsets;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Switch;
import android.widget.TextView;

public final class MainActivity extends Activity {
    private static final int PURPLE=0xFF8174C9, INK=0xFF303044, LIGHT=0xFFF7F5FD;
    private static final String CONTROL="com.gy.gyglass.lsp.CONTROL";
    private static final String REPLY="com.gy.gyglass.lsp.REPLY";
    private TextView status, opacityLabel;
    private Switch enabled;
    private RadioGroup modes;
    private SeekBar opacity;
    private final Handler handler=new Handler(Looper.getMainLooper());
    private boolean connected=false;
    private final BroadcastReceiver receiver=new BroadcastReceiver() {
        @Override public void onReceive(Context c,Intent intent) {
            if(!REPLY.equals(intent.getAction()))return;
            connected=true;
            boolean ready=intent.getBooleanExtra("ready",false);
            boolean applied=intent.getBooleanExtra("applied",false);
            status.setText(applied?(ready?"已应用到输入法进程 · Hook 已挂载":"已保存到输入法进程 · 渲染 Hook 未就绪"):
                ready?"LSPosed Hook 已加载（效果仍需实际检查）":"已连接输入法进程，但渲染 Hook 未就绪");
        }
    };

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        getWindow().setStatusBarColor(LIGHT);
        getWindow().setNavigationBarColor(LIGHT);
        getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR|View.SYSTEM_UI_FLAG_LIGHT_NAVIGATION_BAR);
        ScrollView scroll=new ScrollView(this);
        scroll.setFillViewport(true);scroll.setBackgroundColor(LIGHT);
        LinearLayout root=new LinearLayout(this);root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(23),dp(24),dp(23),dp(35));
        scroll.addView(root);
        scroll.setOnApplyWindowInsetsListener((v,insets)->{
            android.graphics.Insets sys=insets.getInsets(WindowInsets.Type.systemBars());
            root.setPadding(dp(23),dp(24)+sys.top,dp(23),dp(35)+sys.bottom);return insets;
        });
        setContentView(scroll);

        TextView title=text("GyGlass",30,INK,true);root.addView(title);
        TextView subtitle=text("微信输入法 · 独立 LSPosed 玻璃模块",13,0xFF77748D,false);
        subtitle.setPadding(0,dp(5),0,dp(22));root.addView(subtitle);

        LinearLayout diagnostic=card();
        diagnostic.addView(text("注入状态",17,INK,true));
        status=text("尚未检测。请先启用作用域并调出微信输入法。",13,0xFF646079,false);
        status.setPadding(0,dp(12),0,dp(12));diagnostic.addView(status);
        Button check=button("检查 LSPosed 注入");
        check.setOnClickListener(v->ping(false));diagnostic.addView(check);
        root.addView(diagnostic);

        LinearLayout settings=card();
        settings.addView(text("玻璃渲染",17,INK,true));
        enabled=new Switch(this);enabled.setText("启用玻璃效果");
        enabled.setTextColor(INK);enabled.setChecked(getPreferences(0).getBoolean("enabled",true));
        settings.addView(enabled);
        TextView modeText=text("键帽样式",13,0xFF77748D,false);modeText.setPadding(0,dp(15),0,0);settings.addView(modeText);
        modes=new RadioGroup(this);modes.setOrientation(RadioGroup.VERTICAL);
        RadioButton seamless=new RadioButton(this);seamless.setId(1001);seamless.setText("一体式 · 字母键透明");
        RadioButton independent=new RadioButton(this);independent.setId(1002);independent.setText("独立式 · 柔和半透明键帽");
        modes.addView(seamless);modes.addView(independent);
        modes.check(getPreferences(0).getBoolean("independent",false)?1002:1001);
        settings.addView(modes);
        opacityLabel=text("键帽透明度：50%",13,0xFF77748D,false);
        opacityLabel.setPadding(0,dp(12),0,0);settings.addView(opacityLabel);
        opacity=new SeekBar(this);opacity.setMax(100);opacity.setProgress(getPreferences(0).getInt("opacity",50));
        opacityLabel.setText("键帽不透明度："+opacity.getProgress()+"%");
        opacity.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){
            @Override public void onProgressChanged(SeekBar s,int p,boolean user){opacityLabel.setText("键帽不透明度："+p+"%");}
            @Override public void onStartTrackingTouch(SeekBar s){}
            @Override public void onStopTrackingTouch(SeekBar s){}
        });settings.addView(opacity);
        Button apply=button("应用到微信输入法");
        apply.setOnClickListener(v->applySettings());settings.addView(apply);
        root.addView(settings);

        LinearLayout help=card();
        help.addView(text("使用说明",17,INK,true));
        TextView instructions=text("① 在 LSPosed 中开启 GyGlass，作用域只选择微信输入法。\n② 完全结束微信输入法进程后重新调出键盘。\n③ 先显示一次微信输入法，再回到这里检查或应用。\n④ 如提示 Hook 未就绪，导出 LSPosed 日志排查。",13,0xFF646079,false);
        instructions.setPadding(0,dp(12),0,0);instructions.setLineSpacing(dp(5),1f);
        help.addView(instructions);root.addView(help);

        TextView footer=text("GyGlass v35.0 · 不依赖 WeTyper · 不需要 KSU 配套模块",12,0xFF858095,false);
        footer.setGravity(Gravity.CENTER);footer.setPadding(0,dp(14),0,0);root.addView(footer);
        try { registerReceiver(receiver,new IntentFilter(REPLY),Context.RECEIVER_EXPORTED); }
        catch(Throwable t) {status.setText("无法注册检测回执："+t.getClass().getSimpleName());}
    }

    @Override protected void onResume(){super.onResume();handler.postDelayed(()->ping(false),350);}
    private void applySettings(){
        getPreferences(0).edit().putBoolean("enabled",enabled.isChecked())
            .putBoolean("independent",modes.getCheckedRadioButtonId()==1002)
            .putInt("opacity",opacity.getProgress()).apply();
        ping(true);
    }
    private void ping(boolean apply){
        connected=false;
        status.setText(apply?"正在等待输入法确认应用…":"正在检测注入状态…");
        Intent i=new Intent(CONTROL);
        i.setPackage("com.tencent.wetype");
        i.putExtra("cmd",apply?"apply":"check");
        if(apply){
            i.putExtra("enabled",enabled.isChecked());
            i.putExtra("independent",modes.getCheckedRadioButtonId()==1002);
            i.putExtra("opacity",opacity.getProgress());
        }
        try{sendBroadcast(i);}catch(Throwable t){status.setText("广播失败："+t.getClass().getSimpleName());return;}
        handler.postDelayed(()->{
            if(!connected)status.setText(apply?
                "未收到应用确认；请先调出微信输入法，并检查 LSPosed 作用域":"未收到 Hook 回执；需先启用 LSPosed 并启动微信输入法");
        },2100);
    }
    @Override protected void onDestroy(){super.onDestroy();try{unregisterReceiver(receiver);}catch(Throwable ignored){}handler.removeCallbacksAndMessages(null);}
    private LinearLayout card(){
        LinearLayout v=new LinearLayout(this);v.setOrientation(LinearLayout.VERTICAL);
        v.setPadding(dp(19),dp(18),dp(19),dp(18));
        GradientDrawable bg=new GradientDrawable();bg.setColor(Color.WHITE);bg.setCornerRadius(dp(19));v.setBackground(bg);
        LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,-2);lp.bottomMargin=dp(15);v.setLayoutParams(lp);return v;
    }
    private TextView text(String s,int sp,int color,boolean bold){
        TextView v=new TextView(this);v.setText(s);v.setTextSize(sp);v.setTextColor(color);
        if(bold)v.setTypeface(Typeface.DEFAULT,Typeface.BOLD);return v;
    }
    private Button button(String s){
        Button v=new Button(this);v.setText(s);v.setTextColor(Color.WHITE);v.setAllCaps(false);
        GradientDrawable bg=new GradientDrawable();bg.setColor(PURPLE);bg.setCornerRadius(dp(13));v.setBackground(bg);
        return v;
    }
    private int dp(int d){return (int)(getResources().getDisplayMetrics().density*d+0.5f);}
}
