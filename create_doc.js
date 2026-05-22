const fs = require("fs");
const {
  Document, Packer, Paragraph, TextRun, Header, Footer,
  AlignmentType, LevelFormat, HeadingLevel, BorderStyle,
  PageBreak, PageNumber, TableOfContents
} = require("docx");

// ---- Content strings (all in one place, single quotes for JS) ----

const introText = '当前，我国正处于“十五五”规划开局起步的关键时期，世界正经历百年未有之大变局，中华民族伟大复兴进入不可逆转的历史进程。在这个波澜壮阔的时代洪流中，作为新时代青年，我们既要深刻认清国际国内发展大势，更要在时代坐标中找准自己的定位。本文立足全国两会精神，结合个人专业学习、实践锻炼与职业规划，回顾入党动机从朴素到坚定、从感性到理性的转变过程，以期进一步强化对党的宗旨的理解与认同，为加入党组织筑牢思想根基与实践基础。';

const p11Text = '当前国际格局正经历深刻调整，大国博弈日趋激烈，地缘政治冲突持续升温，全球化发展遇到逆风。在这样的大背景下，中国始终保持战略定力，坚持和平发展道路，积极推动构建人类命运共同体。国内方面，我国经济社会发展迈上新台阶，高质量发展成为主题，新质生产力加快发展，科技创新引领产业变革，这些都为青年一代提供了广阔的发展舞台。同时，我们也要清醒看到，发展不平衡不充分问题仍然突出，关键核心技术“卡脖子”问题待突破，这些都需要年轻一代发扬担当精神，勇于投身科技创新和产业升级的实践中去。';

const p12Text = '2026年全国两会是在“十五五”规划开局之年召开的重要会议。两会聚焦高质量发展，强调科技自立自强、教育科技人才一体化发展、全面深化改革等核心议题。会议指出，要加快发展新质生产力，培育壮大新增长极，推动产业链供应链优化升级。这些都与青年一代的成长发展紧密相关。作为发展对象，我们要深刻学习两会精神，把个人理想追求与国家发展战略紧密结合，在服务国家发展大局中实现人生价值。';

const p13Text = '“十五五”时期是我国全面建设社会主义现代化国家的关键时期，是实现中华民族伟大复兴的重要阶段。规划明确了未来五年经济社会发展的主要目标和重点任务，强调创新驱动、绿色转型、共同富裕等核心理念。这一规划不仅为国家发展指明了方向，也为每一个青年人的成长提供了宏大背景和时代机遇。我们要深刻理解“十五五”规划的重大意义，自觉将个人发展融入国家发展大局。';

const p21Text = '历史的接力棒已经传递到当代青年手中。党的二十大报告指出：“当代中国青年生逢其时，施展才干的舞台无比广阔，实现梦想的前景无比光明。”新时代青年既要有家国情怀，也要有国际视野；既要有书生意气，也要有实干精神。作为发展对象，我们更应走在前列，以先进模范为榜样，主动担当起时代赋予的历史使命。';

const p22Text = '作为新时代的大学生，专业学习是我们的立身之本。在学习过程中，要不仅掌握扎实的专业知识，更要培养创新思维和解决实际问题的能力。无论是理工科学生还是文史哲类学生，都应将专业学习与国家发展需求紧密联系。要关注国家重大战略需求，在自己的学科领域深耕细作，力争在将来的工作岗位上发挥专业特长，为国家发展贡献青春力量。同时，要积极参与社会实践，在实践中检验所学、增长才干。';

const p23Text = '职业规划不是纯粹的个人设计，而应当与国家发展需求同向而行。要关注国家产业发展方向，了解行业前沿动态，在职业选择中体现家国情怀和社会责任。无论是投身科研创新、还是服务基层，都是为人民服务的重要岗位。要把个人理想融入党和国家的事业之中，在服务国家发展大局中找准职业定位，实现人生价值和社会价值的统一。';

const p31Text = '回顾最初的入党动机，很多同学都是基于对党的朴素情感——或是受家庭长辈的熏陶，或是被身边优秀党员的事迹所感染，或是在学习中对党的辉煌历史产生崇敬。这份朴素的情感是可贵的，但还不够深刻。随着学习的深入和思想的成熟，我逐渐意识到，加入党组织不是一种荣誉的加冠，而是一份沉甸甸的责任担当。党的宗旨是全心全意为人民服务，党员的价值在于奉献而非索取。这种从朴素到理性的转变，是思想觉悟的重要里程碑。';

const p32Text = '感性认识往往基于情感共鸣，而坚定信念则源于深刻理解。通过系统学习党的理论知识、学习党的历史、学习党纪党规，我对党的认识从表面走向深入。特别是通过参加党校培训、志愿服务、社会实践等活动，更直观地感受到党的理论与实践的统一，感受到党的路线方针政策的正确性。这种从感性到理性的飞跃，使我的入党动机更加纯洁、更加坚定。我深刻认识到，只有中国共产党才能领导中国人民实现民族复兴，只有中国特色社会主义才能发展中国。这份坚定的信念，成为我不断前进的动力源泉。';

const p33Text = '全心全意为人民服务，这是党的根本宗旨。这句话说起来容易，但真正理解它的深刻内涵并贯彻到行动中，需要一生的修为。在学习和实践中，我逐渐体会到，为人民服务不是一句口号，而是体现在点点滴滴的行动中。在学习上帮助同学，在生活中关心他人，在社会实践中服务群众，这些都是贯彻党的宗旨的具体体现。只有把全心全意为人民服务内化为自己的价值追求和行动自觉，才能真正贯彻好党的宗旨。';

const p41Text = '思想上入党，首先要解决信仰问题。要坚持不懈地学习党的创新理论，特别是习近平新时代中国特色社会主义思想，用党的创新理论武装头脑、指导实践。要养成阅读党报党刊、关注时事政治的习惯，在学习中提高政治站位，增强“四个意识”、坚定“四个自信”、做到“两个维护”。同时，要经常开展批评与自我批评，在反思中不断提升思想觉悟，使自己的思想边界越来越靠近党员标准。';

const p42Text = '党性不是天生的，而是在实践中淬炼出来的。要积极参加各类实践活动，在志愿服务中体会奉献的乐趣，在社会调研中了解国情民意，在基层锻炼中增长才干。要主动承担急难险重任务，在服务同学、服务学校、服务社会的过程中，不断提升自己的组织能力、协调能力和解决问题的能力。要坚持理论联系实际的学风，把所学知识转化为服务人民群众的本领。';

const p43Text = '要时刻以党员标准要求自己，在学习、工作和生活中起到先锋模范作用。在学习上刻苦努力，争取优异成绩；在工作中勇于担当，主动承担急难险重任务；在生活中严于律己，自觉遵守校纪校规和社会公德。要经常用党章这面镜子对照检查自己，用党纪这把尺子严格约束自己，不断向党员标准看齐。要时刻牢记，组织上入党一生一次，思想上入党一生一世，以此警醒自己不断前进、永不停歇。';

const conclusionText = '青春向党，时代向上。站在“十五五”开局起步的新起点上，我深感责任重大、使命光荣。通过这次深入的思想汇报，我更加清晰地认识到，加入中国共产党不是一时的冲动，而是一生的承诺。我将继续以实际行动践行入党誓言，在学习中进步，在实践中锻炼，在奉献中成长，以更加饱满的热情和更加务实的作风，为早日成为一名合格的共产党员而努力奋斗。';

const bulletPoints = [
  '第一，坚持理论学习不松劲，用习近平新时代中国特色社会主义思想武装头脑，提高政治素养和理论水平。',
  '第二，勇担时代责任不退缩，立足专业学习，在实践中增长才干，在服务中奉献青春。',
  '第三，时刻对标党员标准不松懈，严于律己，接受组织考验，争取早日成为一名合格的共产党员。',
  '第四，牢记初心使命不动摇，始终保持对党的忠诚和对人民的深情，在为人民服务的道路上坚定前行。'
];

// ---- Helper functions ----

const makeRun = (text, options = {}) => new TextRun({ text, size: 24, font: "SimSun", ...options });

const h1 = (text) => new Paragraph({
  heading: HeadingLevel.HEADING_1,
  children: [new TextRun({ text, font: "SimHei" })]
});

const h2 = (text) => new Paragraph({
  heading: HeadingLevel.HEADING_2,
  children: [new TextRun({ text, font: "SimHei" })]
});

const p = (text) => new Paragraph({
  spacing: { after: 200 },
  children: [makeRun(text)]
});

const pageBreak = () => new Paragraph({ children: [new PageBreak()] });

const headerText = '担青年时代使命  正入党初心航向';

const pageProps = {
  page: {
    size: { width: 11906, height: 16838 },
    margin: { top: 1440, right: 1440, bottom: 1440, left: 1440 }
  }
};

const defaultHeader = new Header({
  children: [new Paragraph({
    alignment: AlignmentType.CENTER,
    children: [new TextRun({ text: headerText, font: "SimHei", size: 18, color: "888888" })]
  })]
});

const defaultFooter = new Footer({
  children: [new Paragraph({
    alignment: AlignmentType.CENTER,
    children: [new TextRun("Page "), new TextRun({ children: [PageNumber.CURRENT] })]
  })]
});

// ---- Build document ----

const doc = new Document({
  styles: {
    default: { document: { run: { font: "SimSun", size: 24 } } },
    paragraphStyles: [
      {
        id: "Heading1", name: "Heading 1", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 32, bold: true, font: "SimHei" },
        paragraph: { spacing: { before: 360, after: 240 }, outlineLevel: 0 }
      },
      {
        id: "Heading2", name: "Heading 2", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 28, bold: true, font: "SimHei" },
        paragraph: { spacing: { before: 240, after: 180 }, outlineLevel: 1 }
      },
    ]
  },
  numbering: {
    config: [{
      reference: "bullets",
      levels: [{
        level: 0, format: LevelFormat.BULLET, text: "•", alignment: AlignmentType.LEFT,
        style: { paragraph: { indent: { left: 720, hanging: 360 } } }
      }]
    }]
  },
  sections: [
    // ===== COVER PAGE =====
    {
      properties: pageProps,
      children: [
        new Paragraph({ spacing: { before: 3600 }, children: [] }),
        new Paragraph({
          alignment: AlignmentType.CENTER,
          spacing: { after: 600 },
          children: [new TextRun({ text: '担青年时代使命', font: "SimHei", size: 52, bold: true, color: "C41E3A" })]
        }),
        new Paragraph({
          alignment: AlignmentType.CENTER,
          spacing: { after: 600 },
          children: [new TextRun({ text: '正入党初心航向', font: "SimHei", size: 52, bold: true, color: "C41E3A" })]
        }),
        new Paragraph({ spacing: { before: 600 }, children: [] }),
        new Paragraph({
          alignment: AlignmentType.CENTER,
          spacing: { after: 200 },
          border: { top: { style: BorderStyle.SINGLE, size: 6, color: "C41E3A", space: 1 } },
          children: []
        }),
        new Paragraph({
          alignment: AlignmentType.CENTER,
          spacing: { before: 400, after: 200 },
          children: [new TextRun({ text: '立足“十五五”开局起步之际  结合全国两会精神', font: "SimHei", size: 28, color: "333333" })]
        }),
        new Paragraph({
          alignment: AlignmentType.CENTER,
          spacing: { after: 200 },
          children: [new TextRun({ text: '发展对象思想汇报专题', font: "SimHei", size: 28, color: "333333" })]
        }),
        new Paragraph({ spacing: { before: 2400 }, children: [] }),
        new Paragraph({
          alignment: AlignmentType.CENTER,
          spacing: { after: 200 },
          children: [new TextRun({ text: '汇报人：_______________', font: "SimSun", size: 26 })]
        }),
        new Paragraph({
          alignment: AlignmentType.CENTER,
          spacing: { after: 200 },
          children: [new TextRun({ text: '日    期：2026年____月____日', font: "SimSun", size: 26 })]
        }),
      ]
    },

    // ===== TOC PAGE =====
    {
      properties: pageProps,
      headers: { default: defaultHeader },
      footers: { default: defaultFooter },
      children: [
        h1('目  录'),
        new TableOfContents("Table of Contents", { hyperlink: true, headingStyleRange: "1-2" }),
      ]
    },

    // ===== MAIN CONTENT =====
    {
      properties: pageProps,
      headers: { default: defaultHeader },
      footers: { default: defaultFooter },
      children: [
        // Introduction
        h1('引言：时代召唤与使命担当'),
        p(introText),

        // Part 1
        h1('一、认清时代大势，把握“十五五”开局的历史方位'),
        h2('（一）国际国内新形势的深刻变化'),
        p(p11Text),
        h2('（二）全国两会精神的核心要义'),
        p(p12Text),
        h2('（三）“十五五”规划的重大意义'),
        p(p13Text),

        // Part 2
        h1('二、青年使命担当，新时代的青春力量'),
        h2('（一）新时代青年的历史责任'),
        p(p21Text),
        h2('（二）结合专业学习，勇担时代责任'),
        p(p22Text),
        h2('（三）职业规划与国家发展的同频共振'),
        p(p23Text),

        // Part 3
        h1('三、入党动机的成长与转变'),
        h2('（一）从朴素情感到理性认同'),
        p(p31Text),
        h2('（二）从感性认识到坚定信念'),
        p(p32Text),
        h2('（三）对党的宗旨的理解与认同'),
        p(p33Text),

        // Part 4
        h1('四、以实际行动向党员标准看齐'),
        h2('（一）思想成长：筑牢信仰之基'),
        p(p41Text),
        h2('（二）实践锻炼：在服务中淬炼党性'),
        p(p42Text),
        h2('（三）行动标准：对标先进、争做先锋'),
        p(p43Text),

        // Conclusion
        h1('结语：不忘初心、牢记使命'),
        p(conclusionText),
        new Paragraph({ spacing: { before: 200 }, children: [] }),
        new Paragraph({
          spacing: { after: 200 },
          children: [new TextRun({ text: '展望未来，我将做到以下几点：', font: "SimHei", size: 26, bold: true })]
        }),
        ...bulletPoints.map((text) => new Paragraph({
          numbering: { reference: "bullets", level: 0 },
          spacing: { after: 120 },
          children: [makeRun(text)]
        })),
        new Paragraph({ spacing: { before: 400 }, children: [] }),
        new Paragraph({
          alignment: AlignmentType.RIGHT,
          spacing: { after: 200 },
          children: [new TextRun({ text: '汇报人：_______________', font: "SimSun", size: 26 })]
        }),
        new Paragraph({
          alignment: AlignmentType.RIGHT,
          spacing: { after: 200 },
          children: [new TextRun({ text: '2026年____月____日', font: "SimSun", size: 26 })]
        }),
      ]
    },
  ]
});

const outputPath = 'D:\\huosheng\\入党思想汇报_担青年时代使命正入党初心航向.docx';
Packer.toBuffer(doc).then(buffer => {
  fs.writeFileSync(outputPath, buffer);
  console.log('SUCCESS: ' + outputPath);
}).catch(err => {
  console.error('ERROR:', err);
});
